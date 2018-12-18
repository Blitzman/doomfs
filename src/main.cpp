#include <cstdint>
#include <fstream>
#include <iostream>

#define WAD_TYPE_LENGTH 4
#define WAD_LUMP_COUNT_LENGTH 4
#define WAD_DIRECTORY_OFFSET_LENGTH 4

short read_short(const uint8_t* pData, unsigned int offset)
{
	short tmp_ = (pData[offset + 1] << 8) | pData[offset];
	return tmp_;
}

unsigned short read_ushort(const uint8_t* pData, unsigned int offset)
{
	unsigned short tmp_ = (pData[offset + 1] << 8) | pData[offset];
	return tmp_;
}

int read_int(const uint8_t* pData, unsigned int offset)
{
	int tmp_ = (pData[offset + 3] << 24) |
							(pData[offset + 2] << 16)	|
							(pData[offset + 1] << 8) |
							pData[offset];
	return tmp_;
}

unsigned int read_uint(const uint8_t* pData, unsigned int offset)
{
	unsigned int tmp_ = (pData[offset + 3] << 24) |
												(pData[offset + 2] << 16) |
												(pData[offset + 1] << 8) |
												pData[offset];

	return tmp_;
}

void copy_and_capitalize_buffer(std::string & rDst, const uint8_t* pSrc, unsigned int offset, unsigned int srcLength)
{
	rDst = "";

	for (unsigned int i = 0; i < srcLength && pSrc[offset + i] != 0; ++i)
		rDst += toupper(pSrc[offset + i]);	
}

struct WADHeader
{
	std::string type;
	unsigned int lump_count;
	unsigned int directory_offset;
};

int main(void)
{
	std::ifstream wad_file_("doom1.wad", std::ios::binary | std::ios::ate);
	std::streamsize wad_size_ = wad_file_.tellg();

	std::cout << "WAD file size is " << wad_size_ << "\n";

	uint8_t* wad_data_ = new uint8_t[(unsigned int)wad_size_];
	wad_file_.seekg(0, std::ios::beg);
	wad_file_.read((char*)wad_data_, wad_size_);

	// Read WAD type (IWAD or PWAD)
	unsigned int offset_ = 0;
	WADHeader wad_header_;
	
	copy_and_capitalize_buffer(wad_header_.type, wad_data_, offset_, WAD_TYPE_LENGTH);
	offset_ += WAD_TYPE_LENGTH;

	wad_header_.lump_count = read_uint(wad_data_, offset_);
	offset_ += WAD_LUMP_COUNT_LENGTH;

	wad_header_.directory_offset = read_uint(wad_data_, offset_);
	
	std::cout << "WAD Header:\n";
	std::cout << "Type: " << wad_header_.type << "\n";
	std::cout << "Lump Count: " << wad_header_.lump_count << "\n";
	std::cout << "Directory Offset: " << wad_header_.directory_offset << "\n";

	if (wad_data_)
	{
		delete[] wad_data_;
		wad_data_ = NULL;
	}
}
