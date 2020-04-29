// Performs analog injection on the 3 RD53A DiffFE columns available for readout(top pads) with chip settings read from a .json file. It will also save the congifuration to another user defined file
// ./bin/rd53a_ana_inj_json (json file to read setting from) (name of file to save settings to)

#include <iostream>
#include <chrono>
#include <unistd.h>
#include <fstream>
#include "SpecController.h"
#include "Rd53a.h"
#include "json.hpp"

using json=nlohmann::basic_json<std::map, std::vector, std::string, bool, std::int32_t, std::uint32_t, float>;


int main(int argc, char *argv[]) {

    int specNum = 0;
  //code below will need to change when running multiple boards (at the same time, e.g. sepcNum=1 for 2nd board etc.)
  //  if (argc > 1)
  //      specNum = std::stoi(argv[1]);

    SpecController spec;
    spec.init(specNum);
    spec.setTrigEnable(0);
    

    spec.writeSingle(0x6<<14 | 0x0, 0x080000);
    spec.writeSingle(0x6<<14 | 0x1, 0xF);
    spec.setCmdEnable(0x1);
    spec.setRxEnable(0x0);
 

//###################################
//#loading chip config from json file

 Rd53a fe(&spec);
    fe.setChipId(0);


 	// Read config file if there
	std::fstream cfgFile;		
	if (argc > 1) {
		cfgFile.open(argv[1], std::ios::in);
		json j;
		cfgFile >> j;
		fe.fromFileJson(j);
		
	}	
  std::cout << ">>> Configuring chip with " << argv[1] << std::endl;
  std::this_thread::sleep_for(std::chrono::milliseconds(100));


    fe.configure();

    while (!spec.isCmdEmpty()) {} //waits for the commands to be written into the registers
	
	// Write config to user specified file
	std::fstream outFile(argv[2], std::ios::out);
	json k;
	fe.toFileJson(k);
	outFile << std::setw(4) << k;
	outFile.close();
//#end of loading chip config and writing settings to user defined file
//#####################################################################

    std::cout << " ... done." << std::endl;
    std::this_thread::sleep_for(std::chrono::milliseconds(10));


    for (unsigned col=0; col<Rd53a::n_Col; col++) {
        for(unsigned row=0; row<Rd53a::n_Row; row++) {
            fe.setEn(col, row, 0); // Disable all to avoid noise
		fe.setInjEn(col , row, 0);
        }
    }

    fe.configure();
    spec.setRxEnable(0x1);

    std::cout << ">>> Enabling injection" << std::endl;
        //enabeling columns 347, 270, 307 for injection
        fe.setEn(347, 0, 1);
	fe.setInjEn(347, 0, 1); 

        fe.setEn(270, 0, 1);
	fe.setInjEn(270, 0, 1); 

        fe.setEn(307, 0, 1);
	fe.setInjEn(307, 0, 1); 
 

      	fe.configurePixels(); 
    	while (!spec.isCmdEmpty()) {} //waits for the commands to be written into the registers

 for (int i=0; i<100; i++) {
    	std::cout << ">>> Injecting" << i+1 << std::endl;
       	
	fe.cal(0, 0, 0, 1, 0, 0); // Inject High -> Med
       	std::this_thread::sleep_for(std::chrono::milliseconds(1));
        fe.cal(0, 1, 0, 0, 0, 0); // Rearm cal_edge
	std::this_thread::sleep_for(std::chrono::milliseconds(1)); 

       	std::this_thread::sleep_for(std::chrono::milliseconds(100));
}

       	std::this_thread::sleep_for(std::chrono::milliseconds(100));
       	

   while (!spec.isCmdEmpty()) {}
    
    //spec.setTrigEnable(0);
    spec.setRxEnable(0x0);
    return 0;
}
