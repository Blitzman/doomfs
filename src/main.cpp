#include <cstdint>
#include <fstream>
#include <iostream>

#include "wad.hpp"

#define WAD_FILENAME "doom1.wad"

int main(void)
{
	WAD wad_(WAD_FILENAME);
	std::cout << wad_;	
}
