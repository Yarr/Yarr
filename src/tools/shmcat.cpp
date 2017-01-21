
#include "EmuShm.h"
#include <iostream>

int main(int argc, const char** argv) {

	EmuShm* tx = new EmuShm(1337,256,0);
	EmuShm* rx = new EmuShm(1338,256,0);

	std::cout << "tx " << std::string('-',60) << std::endl;
	tx->dump();
	std::cout << "tx " << std::string('-',60) << std::endl;
	std::cout << "rx " << std::string('-',60) << std::endl;
	rx->dump();
	std::cout << "rx " << std::string('-',60) << std::endl;

	return EXIT_SUCCESS;
}
