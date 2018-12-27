#ifndef WAD_HPP_
#define WAD_HPP_

#include <cassert>
#include <fstream>
#include <iostream>
#include <map>
#include <memory>
#include <ostream>
#include <vector>

#include "ppm_writer.hpp"
#include "readers.hpp"

#define WAD_HEADER_TYPE_LENGTH 4
#define WAD_HEADER_LUMPCOUNT_LENGTH 4
#define WAD_HEADER_OFFSET_LENGTH 4
#define WAD_ENTRY_OFFSET_LENGTH 4
#define WAD_ENTRY_SIZE_LENGTH 4
#define WAD_ENTRY_NAME_LENGTH 8

// http://www.gamers.org/dhs/helpdocs/dmsp1666.html

struct WADHeader
{
	std::string type;
	unsigned int lump_count;
	unsigned int directory_offset;
};

struct WADEntry
{
	unsigned int offset;
	unsigned int size;
	std::string name;
};

struct WADPaletteColor
{
	uint8_t r;
	uint8_t g;
	uint8_t b;
};

struct WADSpritePost
{
  uint8_t col;
  uint8_t row;
  uint8_t size;
  std::vector<uint8_t> pixels;
};

struct WADSprite
{
  unsigned int width;
  unsigned int height;
  unsigned int left_offset;
  unsigned int top_offset;
  std::vector<WADSpritePost> posts;
};

class WAD
{
	public:

		WAD(const std::string & filename)
		{
			m_offset = 0;

			// Load the whole WAD file into memory, it only takes a few MiBs
			load_wad(filename);

			read_header();

			// Pre-allocate directory since we know its size from the header and then read it
			m_directory.reserve(m_wad_header.lump_count);
			read_directory();

      // Pre-allocate the 14 palettes that original DOOM uses and read them
      m_palettes.reserve(14);
			read_palettes();
      std::cout << "Read " << m_palettes.size() << " palettes...\n";
			write_palettes();

      // Pre-allocate the 34 colormaps that original DOOM uses and read them
      m_colormaps.reserve(34);
      read_colormaps();
      std::cout << "Read " << m_colormaps.size() << " color maps...\n";
      write_colormaps();

      read_sprites();
      std::cout << "Read " << m_sprites.size() << " sprites...\n";
      write_sprites();
		}

		friend std::ostream& operator<<(std::ostream& rOs, const WAD& rWad)
		{
			rOs << "WAD file\n";
			rOs << "Type: " << rWad.m_wad_header.type << "\n";
			rOs << "Lump Count: " << rWad.m_wad_header.lump_count << "\n";
			rOs << "Directory Offset: " << rWad.m_wad_header.directory_offset << "\n";

			//rOs << "Directory:\n";

			//for (unsigned int i = 0; i < rWad.m_wad_header.lump_count; ++i)
			//	rOs << rWad.m_directory[i].name << " " << rWad.m_directory[i].size << "\n";
		}

	private:

		void load_wad(const std::string & filename)
		{
			std::cout << "Reading WAD " << filename << "\n";
			std::ifstream wad_file_(filename, std::ios::binary | std::ios::ate);

			if (!wad_file_)
				throw std::runtime_error("Could not open file " + filename);

			std::streamsize wad_size_ = wad_file_.tellg();
			std::cout << "WAD file size is " << wad_size_ << "\n";

			m_wad_data = std::make_unique<uint8_t[]>((unsigned int)wad_size_);

			wad_file_.seekg(0, std::ios::beg);
			wad_file_.read((char*)m_wad_data.get(), wad_size_);
			wad_file_.close();

			std::cout << "WAD read successfully!\n";

			m_offset = 0;
		}

		void read_header()
		{
			assert(m_wad_data);

			// The header is a 12-byte component which consists of three 4-byte parts:
			//	(a) an ASCII string (4-byte) which is "IWAD" or "PWAD"
			//	(b)	an unsigned int (4-byte) to hold the number of lumps in the WAD file
			//	(c) an unsigned int (4-byte) that indicates the file offset to the start of the directory

			copy_and_capitalize_buffer(m_wad_header.type, m_wad_data, m_offset, WAD_HEADER_TYPE_LENGTH);
			m_wad_header.lump_count = read_uint(m_wad_data, m_offset);
			m_wad_header.directory_offset = read_uint(m_wad_data, m_offset);
		}

		void read_directory()
		{
			assert(m_wad_data);
			assert(m_wad_header.lump_count != 0);

			// The directory has one 16-byte entry for every lump. Each entry consists of three parts:
			//	(a) an unsigned int (4-byte) which indicates the file offset to the start of the lump
			//	(b) an unsigned int (4-byte) which indicates the size of the lump in bytes
			//	(c) an ASCII string (8-byte) which holds the name of the lump (padded with zeroes)

			m_offset = m_wad_header.directory_offset;
			for (unsigned int i = 0; i < m_wad_header.lump_count; ++i)
			{
				WADEntry entry_;
				entry_.offset = read_uint(m_wad_data, m_offset);
				entry_.size = read_uint(m_wad_data, m_offset);
				copy_and_capitalize_buffer(entry_.name, m_wad_data, m_offset, 8);

				m_directory.push_back(entry_);
				m_lump_map.insert(std::pair<std::string, unsigned int>(entry_.name, i));
			}
		}

		void read_palettes()
		{
			assert(m_wad_data);
			assert(m_lump_map.find("PLAYPAL") != m_lump_map.end());

			// Palettes are found in the PLAYPAL lump. There are 14 palettes, each is 768 bytes (since
			// they are composed of 256 RGB triplets, and each RGB value is a 1-byte from 0 to 255).

			WADEntry palettes_ = m_directory[m_lump_map["PLAYPAL"]];

			m_offset = palettes_.offset;
			while (m_offset < palettes_.offset + palettes_.size)
			{
				std::vector<WADPaletteColor> palette_(256);

				for (unsigned int i = 0; i < 256; ++i)
				{
					WADPaletteColor color_;
					color_.r = m_wad_data[m_offset++];
					color_.g = m_wad_data[m_offset++];
					color_.b = m_wad_data[m_offset++];
					palette_[i] = color_;
				}

				m_palettes.push_back(palette_);
			}
		}

		void write_palettes()
		{
			assert(m_palettes.size() != 0);

			PPMWriter writer_;

			for (unsigned int i = 0; i < m_palettes.size(); ++i)
				writer_.write<WADPaletteColor>(m_palettes[i], 16, 16, "palette" + std::to_string(i) + ".ppm");
		}

    void read_colormaps()
    {
      assert(m_wad_data);
      assert(m_lump_map.find("COLORMAP") != m_lump_map.end());

      // Color maps are found in the COLORMAP lump. There are 34 color maps, each is 256 bytes (each
      // byte in each color map indicates the number of the palette color to which the original color
      // gets mapped, e.g., byte 2 in color map 3 indicates to which original color 2 gets mapped).

      WADEntry colormaps_ = m_directory[m_lump_map["COLORMAP"]];

      m_offset = colormaps_.offset;
      while (m_offset < colormaps_.offset + colormaps_.size)
      {
        std::vector<uint8_t> colormap_(256);

        for (unsigned int i = 0; i < 256; ++i)
          colormap_[i] = m_wad_data[m_offset++];

        m_colormaps.push_back(colormap_);
      }
    }

    void write_colormaps()
    {
      assert(m_colormaps.size() != 0);

      PPMWriter writer_;

      std::vector<WADPaletteColor> colormap_img_(256 * m_palettes.size() * m_colormaps.size());
      const unsigned int width_ = 256;
      const unsigned int height_ = m_colormaps.size() * m_palettes.size();

      for (unsigned int i = 0; i < m_colormaps.size(); ++i)
      {
        for (unsigned int k = 0; k < m_palettes.size(); ++k)
        {
          for (unsigned int l = 0; l < 256; ++l)
          {
            colormap_img_[((k * m_colormaps.size()) + i) * 256 + l] = m_palettes[k][(unsigned int)m_colormaps[i][l]];
          }
        }
      }

      writer_.write<WADPaletteColor>(colormap_img_, m_colormaps.size()  * m_palettes.size(), 256, "colormaps.ppm");
    }

    void read_sprites()
    {
      assert(m_wad_data);

      std::vector<std::string> sprite_names_ { "SUITA0", "TROOA1", "BKEYA0" };

      for (std::string s : sprite_names_)
      {
        assert(m_lump_map.find(s) != m_lump_map.end());
        WADEntry sprite_lump_ = m_directory[m_lump_map[s]];
        m_offset = sprite_lump_.offset;

        WADSprite sprite_;

        // Each picture starts with an 8-byte header of four shorts. Those four fields are the following:
        //  (a) The width of the picture (number of columns of pixels)
        //  (b) The height of the picture (number of rows of pixels)
        //  (c) The left offset (number of pixels to the left of the center where the first column is drawn)
        //  (d) The top offset (number of pixels to the top of the center where the top row is drawn).

        sprite_.width = read_ushort(m_wad_data, m_offset);
        sprite_.height = read_ushort(m_wad_data, m_offset);
        sprite_.left_offset = read_ushort(m_wad_data, m_offset);
        sprite_.top_offset = read_ushort(m_wad_data, m_offset);

        std::cout << "Read sprite " << s << " (" << sprite_.width << ", " << sprite_.height << ", " << sprite_.left_offset << ", " << sprite_.top_offset << ")\n";

        // After the header, there are as many 4-byte integers as columns in the picture. Each one of them
        // is a pointer to the data start for each column (an offset from the first byte of the LUMP)

        std::vector<unsigned int> column_offsets_(sprite_.width);
        for (int i = 0; i < sprite_.width; ++i)
          column_offsets_[i] = read_uint(m_wad_data, m_offset);

        // Each column data is an array of bytes arranged in another structure named POSTS. Each POST has
        // the following structure:
        //  (a) The first byte is the row to start drawing
        //  (b) The second byte is the size of the post (the amount of pixels to draw downwards)
        //  (c) As many bytes as pixels in the post + 2 additional bytes. Each byte defines the color index
        //      in the current game palette that the pixel uses. The first and last bytes of this arrangement
        //      are TO BE IGNORED, THEY ARE NOT DRAWN
        //
        // After the last byte of the POST, there might be another POST with the same structure as before
        // or the column might end. A 255 (0xFF) value after a post indicates that the column ends and the
        // following pixels are transparent. Note that a column may immediately begin with 0xFF and no post
        // at all. In such case, the whole column is transparent.

        for (int i = 0; i < sprite_.width; ++i)
        {
          m_offset = sprite_lump_.offset + column_offsets_[i];

          while (m_wad_data[m_offset] != 0xFF)
          {
            WADSpritePost post_;
            post_.col = i;
            post_.row = m_wad_data[m_offset++];
            post_.size = m_wad_data[m_offset++];

            // Skip the first unused pixel
            m_offset++;

            for (uint8_t k = 0; k < post_.size; ++k)
              post_.pixels.push_back(m_wad_data[m_offset++]);

            // Skip the last unused pixel
            m_offset++;

            sprite_.posts.push_back(post_);
          }
        }

        m_sprites.insert(std::pair<std::string, WADSprite>(s, sprite_));
      }
    }

    void write_sprites()
    {
      assert(m_palettes.size() != 0);
      // TODO: Colormaps are not used in this routine
      assert(m_colormaps.size() != 0);
      assert(m_sprites.size() != 0);

      PPMWriter writer_;

      for (auto s : m_sprites)
      {
        std::string name_ = s.first;
        WADSprite sprite_ = s.second;

        std::vector<WADPaletteColor> texture_(sprite_.width * m_palettes.size() * sprite_.height);

        for (auto p : sprite_.posts)
        {
          unsigned int row_ = p.row;
          unsigned int col_ = p.col;

          for (unsigned int i = 0; i < p.size; ++i)
          {
            for (unsigned int k = 0; k < m_palettes.size(); ++k)
            {
              unsigned int idx_ = (row_ + i) * sprite_.width * m_palettes.size() + (col_ + sprite_.width * k);
              texture_[idx_] = m_palettes[k][p.pixels[i]];
            }
          }
        }

        writer_.write<WADPaletteColor>(texture_, sprite_.height, sprite_.width * m_palettes.size(), name_ + ".ppm");
      }
    }

		unsigned int m_offset;
		std::unique_ptr<uint8_t[]> m_wad_data;

		WADHeader m_wad_header;
		std::vector<WADEntry> m_directory;
		std::map<std::string, unsigned int> m_lump_map; 
		std::vector<std::vector<WADPaletteColor>> m_palettes;
    std::vector<std::vector<uint8_t>> m_colormaps;
    std::map<std::string, WADSprite> m_sprites;
};

#endif
