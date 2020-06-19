#include <string>
#include <iostream>

#include "Histo2d.h"
#include "Histo1d.h"

int main(int argc, char*argv[]) {
	if (argc < 2 || argc > 2) {
		std::cout << "Usage: " << argv[0] << " <filename>" << std::endl;
		return -1;
	}
	Histo1d h1("Temp1", 1, 0.0, 1.0);
	Histo2d h2("Temp2", 1, 0.0, 1.0, 1, 0.0, 1.0);

	if(h1.fromFile(std::string(argv[1]))){
		h1.plot("replot", "./");
	} else if (h2.fromFile(std::string(argv[1]))) {
		h2.plot("replot", "./");
	} else {
		std::cout << "ABORTING: Could not read as either 1D or 2D histogram for replotting" << std::endl;
	}
	return 0;
}