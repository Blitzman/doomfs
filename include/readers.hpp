#ifndef READERS_HPP_
#define READERS_HPP_

#include <cstdint>
#include <memory>

short read_short(const std::unique_ptr<uint8_t[]> & rpData, unsigned int & offset)
{
	short tmp_ = (rpData[offset + 1] << 8) | rpData[offset];
  offset += 2;
	return tmp_;
}

unsigned short read_ushort(const std::unique_ptr<uint8_t[]> & rpData, unsigned int & offset)
{
	unsigned short tmp_ = (rpData[offset + 1] << 8) | rpData[offset];
  offset += 2;
	return tmp_;
}

int read_int(const std::unique_ptr<uint8_t[]> & rpData, unsigned int & offset)
{
	int tmp_ = (rpData[offset + 3] << 24) |
							(rpData[offset + 2] << 16)	|
							(rpData[offset + 1] << 8) |
							rpData[offset];
  offset += 4;
	return tmp_;
}

unsigned int read_uint(const std::unique_ptr<uint8_t[]> & rpData, unsigned int & offset)
{
	unsigned int tmp_ = (rpData[offset + 3] << 24) |
												(rpData[offset + 2] << 16) |
												(rpData[offset + 1] << 8) |
												rpData[offset];
  offset += 4;
	return tmp_;
}

void copy_and_capitalize_buffer(std::string & rDst, const std::unique_ptr<uint8_t[]> & rpSrc, unsigned int & offset, unsigned int srcLength)
{
	rDst = "";

	for (unsigned int i = 0; i < srcLength && rpSrc[offset + i] != 0; ++i)
		rDst += toupper(rpSrc[offset + i]);

  offset += srcLength;
}


#endif
