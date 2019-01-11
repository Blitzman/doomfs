#include <iostream>

#include "application.hpp"
#include "wad.hpp"

#define WAD_FILENAME "doom1.wad"

int main(void)
{
	//WAD wad_(WAD_FILENAME);
	//std::cout << wad_;

	Application app_;
	app_.run();

	return 0;
}