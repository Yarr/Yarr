#include <iostream>
#include <fstream>
#include <iomanip>

#include "SpecController.h"
#include "TxCore.h"
#include "RxCore.h"
#include "json.hpp"

#include "Fe65p2.h"

using json = nlohmann::json;

int main(int argc, char *argv[]) {
	
	SpecController mySpec(0);
	TxCore tx(&mySpec);
	RxCore rx(&mySpec);
	
	tx.setCmdEnable(0x1);
	
	Fe65p2 fe(&tx);
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
