#ifndef WAD_HPP_
#define WAD_HPP_

#include <cassert>
#include <fstream>
#include <iostream>
#include <memory>
#include <ostream>

#include "readers.hpp"

#define WAD_TYPE_LENGTH 4
#define WAD_LUMP_COUNT_LENGTH 4
#define WAD_DIRECTORY_OFFSET_LENGTH 4

struct WADHeader
{
	std::string type;
	unsigned int lump_count;
	unsigned int directory_offset;
};


class WAD
{
	public:

		WAD(const std::string & filename)
		{
			m_offset = 0;
			load_wad(filename);
			read_header();
		}

		friend std::ostream& operator<<(std::ostream& rOs, const WAD& rWad)
		{
			rOs << "WAD file\n";
			rOs << "Type: " << rWad.m_wad_header.type << "\n";
			rOs << "Lump Count: " << rWad.m_wad_header.lump_count << "\n";
			rOs << "Directory Offset: " << rWad.m_wad_header.directory_offset << "\n";
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

			copy_and_capitalize_buffer(m_wad_header.type, m_wad_data, m_offset, WAD_TYPE_LENGTH);
			m_offset += WAD_TYPE_LENGTH;

			m_wad_header.lump_count = read_uint(m_wad_data, m_offset);
			m_offset += WAD_LUMP_COUNT_LENGTH;

			m_wad_header.directory_offset = read_uint(m_wad_data, m_offset);
		}

		WADHeader m_wad_header;
		unsigned int m_offset;
		std::unique_ptr<uint8_t[]> m_wad_data;
};

#endif
