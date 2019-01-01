#ifndef WAD_HPP_
#define WAD_HPP_

#include <cassert>
#include <fstream>
#include <functional>
#include <iostream>
#include <map>
#include <memory>
#include <ostream>
#include <regex>
#include <vector>

#include "ppm_writer.hpp"
#include "readers.hpp"

#define WAD_HEADER_TYPE_LENGTH 4
#define WAD_HEADER_LUMPCOUNT_LENGTH 4
#define WAD_HEADER_OFFSET_LENGTH 4
#define WAD_ENTRY_OFFSET_LENGTH 4
#define WAD_ENTRY_SIZE_LENGTH 4
#define WAD_ENTRY_NAME_LENGTH 8
#define WAD_LEVEL_SECTOR_TEXTURE_NAME_LENGTH 8
#define WAD_LEVEL_SIDEDEF_TEXTURE_NAME_LENGTH 8

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

struct WADLevelThing
{
  unsigned short x;
  unsigned short y;
  unsigned short angle;
  unsigned short type;
  unsigned short options;
};

struct WADLevelLinedef
{
  unsigned short from;
  unsigned short to;
  unsigned short flags;
  unsigned short types;
  unsigned short tag;
  unsigned short right_sidedef;
  unsigned short left_sidedef;
};

struct WADLevelSidedef
{
  unsigned short x_offset;
  unsigned short y_offset;
  std::string upper_texture;
  std::string lower_texture;
  std::string middle_texture;
  unsigned short sector;
};

struct WADLevelVertex
{
  unsigned short x;
  unsigned short y;
};

struct WADLevelSeg
{
  unsigned short start;
  unsigned short end;
  unsigned short angle;
  unsigned short linedef;
  unsigned short direction;
  unsigned short offset;
};

struct WADLevelSubSector
{
  unsigned short num_segs;
  unsigned short start_seg;
};

struct WADLevelNode
{
  unsigned short x_start;
  unsigned short y_start;
  unsigned short dx;
  unsigned short dy;
  unsigned short right_y_upper;
  unsigned short right_y_lower;
  unsigned short right_x_lower;
  unsigned short right_x_upper;
  unsigned short left_y_upper;
  unsigned short left_y_lower;
  unsigned short left_x_lower;
  unsigned short left_x_upper;
  unsigned short right_child;
  unsigned short left_child;
};

struct WADLevelSector
{
  unsigned short floor_height;
  unsigned short ceiling_height;
  std::string floor_texture;
  std::string ceiling_texture;
  unsigned short light_level;
  unsigned short special;
  unsigned short tag;

  //
  // TODO: Before implementing more inline friend functions, analyze the implications in
  // terms of performance and space.
  //
  inline friend std::ostream& operator<<(std::ostream& rOs, const WADLevelSector& crSec)
  {
    rOs << "SECTOR - ";
    rOs << "(" << crSec.floor_height << ", " << crSec.ceiling_height << ")\n";
    rOs << "Floor texture: " << crSec.floor_texture << "\n";
    rOs << "Ceiling texture: " << crSec.ceiling_texture << "\n";
    rOs << "Light level: " << crSec.light_level << "\n";
    rOs << "Special: " << crSec.special << "\n";
    rOs << "Tag: " << crSec.tag << "\n";
    rOs << "\n";
  }
};

struct WADLevelBlockmap
{
  unsigned short x;
  unsigned short y;
  unsigned short num_cols;
  unsigned short num_rows;
  std::vector<std::vector<unsigned short>> blocklists;
};

struct WADLevel
{
  std::string name;
  std::vector<WADLevelThing> things;
  std::vector<WADLevelLinedef> linedefs;
  std::vector<WADLevelSidedef> sidedefs;
  std::vector<WADLevelVertex> vertices;
  std::vector<WADLevelSeg> segs;
  std::vector<WADLevelSubSector> ssectors;
  std::vector<WADLevelNode> nodes;
  std::vector<WADLevelSector> sectors;
  WADLevelBlockmap blockmap;
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

      read_levels();
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
			//	(1) an ASCII string (4-byte) which is "IWAD" or "PWAD"
			//	(2)	an unsigned int (4-byte) to hold the number of lumps in the WAD file
			//	(3) an unsigned int (4-byte) that indicates the file offset to the start of the directory

			copy_and_capitalize_buffer(m_wad_header.type, m_wad_data, m_offset, WAD_HEADER_TYPE_LENGTH);
			m_wad_header.lump_count = read_uint(m_wad_data, m_offset);
			m_wad_header.directory_offset = read_uint(m_wad_data, m_offset);
		}

		void read_directory()
		{
			assert(m_wad_data);
			assert(m_wad_header.lump_count != 0);

			// The directory has one 16-byte entry for every lump. Each entry consists of three parts:
			//	(1) an unsigned int (4-byte) which indicates the file offset to the start of the lump
			//	(2) an unsigned int (4-byte) which indicates the size of the lump in bytes
			//	(3) an ASCII string (8-byte) which holds the name of the lump (padded with zeroes)

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
        //  (1) The width of the picture (number of columns of pixels)
        //  (2) The height of the picture (number of rows of pixels)
        //  (3) The left offset (number of pixels to the left of the center where the first column is drawn)
        //  (4) The top offset (number of pixels to the top of the center where the top row is drawn).

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
        //  (1) The first byte is the row to start drawing
        //  (2) The second byte is the size of the post (the amount of pixels to draw downwards)
        //  (3) As many bytes as pixels in the post + 2 additional bytes. Each byte defines the color index
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

    void read_level_things(WADLevel & rLevel, WADEntry entry)
    {
      std::cout << "Reading THINGS\n";

      m_offset = entry.offset;

      while (m_offset < entry.offset + entry.size)
      {
        WADLevelThing thing_;

        // THINGS are generic descriptors for monsters, weapons, keys, barrels, ... Each one of them takes 10 bytes to
        // specify various aspects:
        //  (1) unsigned short (2 bytes) X coordinate position of the THING
        //  (2) unsigned short (2 bytes) Y coordinate position of the THING
        //  (3) unsigned short (2 bytes) angle the THING faces (values rounded to the nearest 45 degree angle)
        //  (4) unsigned short (2 bytes) type of THING
        //  (5) unsigned short (2 bytes) options for the THING

        thing_.x = read_ushort(m_wad_data, m_offset);
        thing_.y = read_ushort(m_wad_data, m_offset);
        thing_.angle = read_ushort(m_wad_data, m_offset);
        thing_.type = read_ushort(m_wad_data, m_offset);
        thing_.options = read_ushort(m_wad_data, m_offset);

        rLevel.things.push_back(thing_);
      }

      std::cout << "Read " << rLevel.things.size() << " THINGS...\n";
    }

    void read_level_linedefs(WADLevel & rLevel, WADEntry entry)
    {
      std::cout << "Reading LINEDEFS\n";

      m_offset = entry.offset;

      while (m_offset < entry.offset + entry.size)
      {
        WADLevelLinedef linedef_;

        // LINEDEFS represent lines from one vertex to another. Each LINEDEF is 14 bytes long and contains seven fields:
        //  (1) unsigned short (2 bytes) the number of the origin vertex
        //  (2) unsigned short (2 bytes) the number of the destination vertex
        //  (3) unsigned short (2 bytes) linedef flags
        //  (4) unsigned short (2 bytes) linedef types
        //  (5) unsigned short (2 bytes) tag to tie this LINEDEF's effect type to a SECTOR
        //  (6) unsigned short (2 bytes) number of the right SIDEDEF to this LINEDEF
        //  (7) unsigned short (2 bytes) number of the left SIDEDEF to this LINEDEF

        linedef_.from = read_ushort(m_wad_data, m_offset);
        linedef_.to = read_ushort(m_wad_data, m_offset);
        linedef_.flags = read_ushort(m_wad_data, m_offset);
        linedef_.types = read_ushort(m_wad_data, m_offset);
        linedef_.tag = read_ushort(m_wad_data, m_offset);
        linedef_.right_sidedef = read_ushort(m_wad_data, m_offset);
        linedef_.left_sidedef = read_ushort(m_wad_data, m_offset);

        rLevel.linedefs.push_back(linedef_);
      }

      std::cout << "Read " << rLevel.linedefs.size() << " LINEDEFS...\n";
    }

    void read_level_sidedefs(WADLevel & rLevel, WADEntry entry)
    {
      std::cout << "Reading SIDEDEFS\n";

      m_offset = entry.offset;

      while (m_offset < entry.offset + entry.size)
      {
        WADLevelSidedef sidedef_;

        // SIDEDEFS are a definition of what wall textures to draw along a LINEDEF so a group of SIDEDEFS outline the
        // space of a SECTOR. Each SIDEDEF is composed of 30 bytes distributed among six fields:
        //  (1) an unsigned short (2 bytes) for the horizontal offset for the texture
        //  (2) an unsigned short (2 bytes) for the vertical offset of the texture
        //  (3) an ASCII string (8 bytes) that indicates the texture name for the upper part of the wall
        //  (4) an ASCII string (8 bytes) that indicates the texture name for the lower part of the wall
        //  (5) an ASCII string (8 bytes) that indicates the texture name for the middle part of the wall
        //  (6) an unsigned short (2 bytes) to reference the SECTOR that this SIDEDEF faces or surrounds

        sidedef_.x_offset = read_ushort(m_wad_data, m_offset);
        sidedef_.y_offset = read_ushort(m_wad_data, m_offset);
        copy_and_capitalize_buffer(sidedef_.upper_texture, m_wad_data, m_offset, WAD_LEVEL_SIDEDEF_TEXTURE_NAME_LENGTH);
        copy_and_capitalize_buffer(sidedef_.lower_texture, m_wad_data, m_offset, WAD_LEVEL_SIDEDEF_TEXTURE_NAME_LENGTH);
        copy_and_capitalize_buffer(sidedef_.middle_texture, m_wad_data, m_offset, WAD_LEVEL_SIDEDEF_TEXTURE_NAME_LENGTH);
        sidedef_.sector = read_ushort(m_wad_data, m_offset);

        rLevel.sidedefs.push_back(sidedef_);
      }

      std::cout << "Read " << rLevel.sidedefs.size() << " SIDEDEFS...\n";
    }

    void read_level_vertexes(WADLevel & rLevel, WADEntry entry)
    {
      std::cout << "Reading VERTEXES\n";

      m_offset = entry.offset;

      while (m_offset < entry.offset + entry.size)
      {
        WADLevelVertex vertex_;

        // VERTEXES are the beginning and the end of SEGS and LINEDEFS. Each vertex is 4 bytes long and contains two fields:
        //  (1) an unsigned short (2 bytes) for the X coordinate
        //  (2) an unsigned short (2 bytes) for the Y coordinate

        vertex_.x = read_ushort(m_wad_data, m_offset);
        vertex_.y = read_ushort(m_wad_data, m_offset);

        rLevel.vertices.push_back(vertex_);
      }

      std::cout << "Read " << rLevel.vertices.size() << " VERTEXES...\n";
    }

    void read_level_segs(WADLevel & rLevel, WADEntry entry)
    {
      std::cout << "Reading SEGS\n";

      m_offset = entry.offset;

      while (m_offset < entry.offset + entry.size)
      {
        WADLevelSeg seg_;

        // SEGS are stored in sequential order determined by the SSECTORS. Each one of them is 12 bytes long with six fields:
        //  (1) an unsigned short (2 bytes) that indicates the start VERTEX
        //  (2) an unsigned short (2 bytes) that indicates the end VERTEX
        //  (3) a signed short (2 bytes) to indicate the angle in BAM format
        //  (4) an unsigned short (2 bytes) that tells the LINEDEF that this SEG goes along
        //  (5) an unsigned short (2 bytes) for the direction of the SEG w.r.t. the LINEDEF (0 - same, 1 - opposite)
        //  (6) an unsigned short (2 bytes) which expresses the distance along the LINEDEF to the start of this SEG

        seg_.start = read_ushort(m_wad_data, m_offset);
        seg_.end = read_ushort(m_wad_data, m_offset);
        seg_.angle = read_ushort(m_wad_data, m_offset);
        seg_.linedef = read_ushort(m_wad_data, m_offset);
        seg_.direction = read_ushort(m_wad_data, m_offset);
        seg_.offset = read_ushort(m_wad_data, m_offset);

        rLevel.segs.push_back(seg_);
      }

      std::cout << "Read " << rLevel.segs.size() << " SEGS...\n";
    }

    void read_level_ssectors(WADLevel & rLevel, WADEntry entry)
    {
      std::cout << "Reading SSECTORS\n";

      m_offset = entry.offset;

      while (m_offset < entry.offset + entry.size)
      {
        WADLevelSubSector ssector_;

        // SSECTORS are sub-sectors that divide the SECTORS into convex polygons. Each SSECTOR is
        // 4 bytes long distributed into two fields:
        //  (1) unsigned short (2 bytes) the amount of SEGS in the SSECTOR
        //  (2) unsigned short (2 bytes) starting SEG number

        ssector_.num_segs = read_ushort(m_wad_data, m_offset);
        ssector_.start_seg = read_ushort(m_wad_data, m_offset);

        rLevel.ssectors.push_back(ssector_);
      }

      std::cout << "Read " << rLevel.ssectors.size() << " SSECTORS...\n";
    }

    void read_level_nodes(WADLevel & rLevel, WADEntry entry)
    {
      std::cout << "Reading NODES\n";

      m_offset = entry.offset;

      while (m_offset < entry.offset + entry.size)
      {
        WADLevelNode node_;

        // NODEs are branches in the binary space partiion that divides the level up. Each NODE has
        // 28 bytes in 14 short fields:
        //  (1) unsigned short (2 bytes) X coordinate of the partition line's start
        //  (2) unsigned short (2 bytes) Y coordinate of the partition line's start
        //  (3) unsigned short (2 bytes) change in X to the end of the partition line
        //  (4) unsigned short (2 bytes) change in Y to the end of the partition line
        //  (5) unsigned short (2 bytes) Y upper bound of the right bounding box
        //  (6) unsigned short (2 bytes) Y lower bound of the right bounding box
        //  (7) unsigned short (2 bytes) X lower bound of the right bounding box
        //  (8) unsigned short (2 bytes) X upper bound of the right bounding box
        //  (9) unsigned short (2 bytes) Y upper bound of the left bounding box
        //  (10) unsigned short (2 bytes) Y lower bound of the left bounding box
        //  (11) unsigned short (2 bytes) X lower bound of the left bounding box
        //  (12) unsigned short (2 bytes) X upper bound of the left bounding box
        //  (13) unsigned short (2 bytes) NODE or SSECTOR number for the right child
        //  (14) unsigned short (2 bytes) NODE or SSECTOR number for the left child

        node_.x_start = read_ushort(m_wad_data, m_offset);
        node_.y_start = read_ushort(m_wad_data, m_offset);
        node_.right_y_upper = read_ushort(m_wad_data, m_offset);
        node_.right_y_lower = read_ushort(m_wad_data, m_offset);
        node_.right_x_lower = read_ushort(m_wad_data, m_offset);
        node_.right_x_upper = read_ushort(m_wad_data, m_offset);
        node_.left_y_upper = read_ushort(m_wad_data, m_offset);
        node_.left_y_lower = read_ushort(m_wad_data, m_offset);
        node_.left_x_lower = read_ushort(m_wad_data, m_offset);
        node_.left_x_upper = read_ushort(m_wad_data, m_offset);
        node_.right_child = read_ushort(m_wad_data, m_offset);
        node_.left_child = read_ushort(m_wad_data, m_offset);

        rLevel.nodes.push_back(node_);
      }

      std::cout << "Read " << rLevel.nodes.size() << " NODES...\n";
    }

    void read_level_sectors(WADLevel & rLevel, WADEntry entry)
    {
      std::cout << "Reading SECTORS\n";

      m_offset = entry.offset;

      while (m_offset < entry.offset + entry.size)
      {
        WADLevelSector sector_;

        // SECTORS are horizonal areas of the map where floor and ceiling heights are defined. Each
        // SECTOR's record is 26 bytes long and it is divided into seven fields:
        //  (1) unsigned short (2 bytes) floor height
        //  (2) unsigned short (2 bytes) ceiling height
        //  (3) ASCII string (8 bytes) name of floor texture
        //  (4) ASCII string (8 bytes) name of ceiling texture
        //  (5) unsigned short (2 bytes) light level for the sector
        //  (6) unsigned short (2 bytes) special flags
        //  (7) unsigned short (2 bytes) tag number of the sector

        sector_.floor_height = read_ushort(m_wad_data, m_offset);
        sector_.ceiling_height = read_ushort(m_wad_data, m_offset);
        copy_and_capitalize_buffer(sector_.floor_texture, m_wad_data, m_offset, WAD_LEVEL_SECTOR_TEXTURE_NAME_LENGTH);
        copy_and_capitalize_buffer(sector_.ceiling_texture, m_wad_data, m_offset, WAD_LEVEL_SECTOR_TEXTURE_NAME_LENGTH);
        sector_.light_level = read_ushort(m_wad_data, m_offset);
        sector_.special = read_ushort(m_wad_data, m_offset);
        sector_.tag = read_ushort(m_wad_data, m_offset);

        rLevel.sectors.push_back(sector_);
      }

      std::cout << "Read " << rLevel.sectors.size() << " SECTORS...\n";
    }

    void read_level_reject(WADLevel & rLevel, WADEntry entry)
    {
      std::cout << "TODO!\n";
    }

    void read_level_blockmap(WADLevel & rLevel, WADEntry entry)
    {
      std::cout << "Reading BLOCKMAP\n";

      m_offset = entry.offset;

      // BLOCKMAPs are pre-computed structures used by the game engine to simplify
      // collision detection between moving things and walls. Each level has one
      // BLOCKMAP and it is divided into three parts: header, offsets, and lists.

      // The header of the BLOCKMAP contains 8 bytes divided into four fields:
      //  (1) unsigned short (2 bytes) the X coordinate of the block grid origin
      //  (2) unsigned short (2 bytes) the Y coordinate of the block grid origin
      //  (3) unsigned short (2 bytes) the number of columns in the map
      //  (4) unsigned short (2 bytes) the number of rows in the map

      rLevel.blockmap.x = read_ushort(m_wad_data, m_offset);
      rLevel.blockmap.y = read_ushort(m_wad_data, m_offset);
      rLevel.blockmap.num_cols = read_ushort(m_wad_data, m_offset);
      rLevel.blockmap.num_rows = read_ushort(m_wad_data, m_offset);

      // After the header, there are N (number of columns in the map times the
      // number of rows) offsets to blocklists. Each offset is a short integer
      // that indicates the starting byte of the corresponding blocklist from
      // the beginning of the BLOCKMAP LUMP.

      unsigned int num_blocks_ = rLevel.blockmap.num_cols * rLevel.blockmap.num_rows;
      std::vector<unsigned short> blocklists_offsets_(num_blocks_);
      for (unsigned int i = 0; i < num_blocks_; ++i)
        blocklists_offsets_.push_back(read_ushort(m_wad_data, m_offset));

      // Each blocklist startrs with a short (0x0000) and ends with another
      // short (0xFFFF). In between there are short indices to LINEDEFs.

      for (unsigned int i = 0; i < blocklists_offsets_.size(); ++i)
      {
        std::vector<unsigned short> blocklist_;

        m_offset = entry.offset + blocklists_offsets_[i];

        // Skip the 0x0000 start of the blocklist
        read_ushort(m_wad_data, m_offset);

        // Start reading numbers until the 0xFFFF ending is found
        unsigned short linedef_index_ = read_ushort(m_wad_data, m_offset);
        while (linedef_index_ != 0xFFFF)
        {
          blocklist_.push_back(linedef_index_);
          linedef_index_ = read_ushort(m_wad_data, m_offset);
        }

        rLevel.blockmap.blocklists.push_back(blocklist_);
      }

      std::cout << "Read " << rLevel.blockmap.blocklists.size() << " lists in the BLOCKMAP...\n";
    }

    void read_levels()
    {
      assert(m_wad_data);
      assert(m_lump_map.size() != 0);
      assert(m_directory.size() != 0);

      // DOOM levels have an ExMy label in the directory (where both x and y are single ASCII digits). The label just
      // indicates that the following LUMPs are part of such level. Actually, the ENTRY for each ExMy does not point to
      // any LUMP and its size is zero.
      
      std::regex level_label_regex_("E[[:digit:]]M[[:digit:]]");

      // Map each possible LUMP name which belongs to a level to the corresponding function to read it

      //
      // TODO: Make this use STD functional
      //
      typedef void (WAD::*read_function_)(WADLevel &, WADEntry);
      std::map<std::string, read_function_> level_lump_names_ {
        {"THINGS", &WAD::read_level_things},
        {"LINEDEFS", &WAD::read_level_linedefs},
        {"SIDEDEFS", &WAD::read_level_sidedefs},
        {"VERTEXES", &WAD::read_level_vertexes},
        {"SEGS", &WAD::read_level_segs},
        {"SSECTORS", &WAD::read_level_ssectors},
        {"NODES", &WAD::read_level_nodes},
        {"SECTORS", &WAD::read_level_sectors},
        {"REJECT", &WAD::read_level_reject},
        {"BLOCKMAP", &WAD::read_level_blockmap},
      };

      for (auto e : m_lump_map)
      {
        std::string lump_name_ = e.first;
        unsigned int directory_index_ = e.second;

        if (std::regex_match(lump_name_, level_label_regex_))
        {
          std::cout << "Found level " << lump_name_ << "\n";

          WADLevel level_;
          level_.name = lump_name_;

          while(level_lump_names_.find(m_directory[++directory_index_].name) != level_lump_names_.end())
          {
            WADEntry entry_ = m_directory[directory_index_];
            (this->*(level_lump_names_[entry_.name]))(level_, entry_);
          }

          m_levels.push_back(level_);
        }
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
    std::vector<WADLevel> m_levels;
};

#endif
