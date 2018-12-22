#ifndef WAD_HPP_
#define WAD_HPP_

#include <cassert>
#include <fstream>
#include <iostream>
#include <memory>
#include <ostream>
#include <vector>

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


class WAD
{
	public:

		WAD(const std::string & filename)
		{
			m_offset = 0;

			// Load the whole WAD file into memory, it only takes a few MiBs
			load_wad(filename);

			read_header();

			// Pre-allocate directory since we know its size from the header
			m_directory.reserve(m_wad_header.lump_count);
			read_directory();
		}

		friend std::ostream& operator<<(std::ostream& rOs, const WAD& rWad)
		{
			rOs << "WAD file\n";
			rOs << "Type: " << rWad.m_wad_header.type << "\n";
			rOs << "Lump Count: " << rWad.m_wad_header.lump_count << "\n";
			rOs << "Directory Offset: " << rWad.m_wad_header.directory_offset << "\n";

			rOs << "Directory:\n";

			for (unsigned int i = 0; i < rWad.m_wad_header.lump_count; ++i)
				rOs << rWad.m_directory[i].name << " " << rWad.m_directory[i].size << "\n";
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
			m_offset += WAD_HEADER_TYPE_LENGTH;

			m_wad_header.lump_count = read_uint(m_wad_data, m_offset);
			m_offset += WAD_HEADER_LUMPCOUNT_LENGTH;

			m_wad_header.directory_offset = read_uint(m_wad_data, m_offset);
			m_offset += WAD_HEADER_OFFSET_LENGTH;
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
				m_offset += WAD_ENTRY_OFFSET_LENGTH;
				entry_.size = read_uint(m_wad_data, m_offset);
				m_offset += WAD_ENTRY_SIZE_LENGTH;
				copy_and_capitalize_buffer(entry_.name, m_wad_data, m_offset, 8);
				m_offset += WAD_ENTRY_NAME_LENGTH;

				m_directory.push_back(entry_);
			}
		}

		WADHeader m_wad_header;
		unsigned int m_offset;
		std::unique_ptr<uint8_t[]> m_wad_data;
		std::vector<WADEntry> m_directory;
};

#endif
