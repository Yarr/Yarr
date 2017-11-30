#include <iostream>
#include <fstream>
#include <iomanip>

#include "SpecController.h"
#include "json.hpp"

#include "Fe65p2.h"

using json=nlohmann::basic_json<std::map, std::vector, std::string, bool, std::int32_t, std::uint32_t, float>;

int main(int argc, char *argv[]) {
	
	SpecController mySpec;
    mySpec.init(0);

	mySpec.setCmdEnable(0x1);
	
	Fe65p2 fe(&mySpec);
	fe.configure();

	// Read config file if there
	std::fstream cfgFile;		
	if (argc > 1) {
		cfgFile.open(argv[1], std::ios::in);
		json j;
		cfgFile >> j;
		fe.fromFileJson(j);
	}	

	// Do stuff to config
	//fe.setValue(&Fe65p2::Vthin1Dac, 100); 
	//fe.setValue(&Fe65p2::Vthin2Dac, 50);

	fe.configureGlobal();

	// Reset all TDACs
	for(unsigned i=0; i<16; i++) {
		fe.PixConf(i).setAll(3);
		//fe.Sign(i).setAll(0);
		//fe.TDAC(i).setAll(0);
	}

	fe.configurePixels();


	// Write config
	std::fstream outFile("fe65p2.json", std::ios::out);
	json k;
	fe.toFileJson(k);
	outFile << std::setw(4) << k;
	outFile.close();
	return 0;
}
