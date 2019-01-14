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

    SpecController spec;
    spec.init(specNum);
    spec.setTrigEnable(0);
    
    spec.writeSingle(0x6<<14 | 0x0, 0x080000);
    spec.writeSingle(0x6<<14 | 0x1, 0xF);
    spec.setCmdEnable(0x1);
    spec.setRxEnable(0x0);
    
    Rd53a fe(&spec);
    fe.setChipId(0);



    std::cout << ">>> Configuring chip with default config ..." << std::endl;
    fe.configure();
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

    //Arming VcalHigh and VcalMed
    fe.writeRegister(&Rd53a::InjVcalHigh, 1300); //2304 for Vdiff = 512 default
    fe.writeRegister(&Rd53a::InjVcalMed, 300); //1792 for Vdiff = 512 default
   
    //Sets Vff
    fe.writeRegister(&Rd53a::DiffVff, 160); 

   //Add/remove needed registers to write to
  /* fe.writeRegister(&Rd53a::DiffFbCapEn, 2); 
   fe.writeRegister(&Rd53a::DiffPrmp, 1023); 
   fe.writeRegister(&Rd53a::DiffFol, 542); 
   fe.writeRegister(&Rd53a::DiffPrecomp, 551);
   fe.writeRegister(&Rd53a::DiffLcc, 20);
   fe.writeRegister(&Rd53a::DiffVth2, 0);
   fe.writeRegister(&Rd53a::DiffVth1, 1023);
   fe.writeRegister(&Rd53a::CalColprDiff1, 65535);
   fe.writeRegister(&Rd53a::CalColprDiff2, 65535);
   fe.writeRegister(&Rd53a::CalColprDiff3, 65535);
   fe.writeRegister(&Rd53a::CalColprDiff4, 65535);
   fe.writeRegister(&Rd53a::CalColprDiff5, 15);    */

    	while (!spec.isCmdEmpty()) {} //waits for the commands to be written into the registers

 fe.configure();

//Enabaling injection for the 3 available columns 270, 307 and 347
    std::cout << ">>> Enabling 1st injection" << std::endl;

        fe.setEn(347, 0, 1);   //enables pixel
	fe.setInjEn(347, 0, 1); //enables injection
        fe.setTDAC(347, 0, 0); //sets TDAC value to zero

        fe.setEn(270, 0, 1);
	fe.setInjEn(270, 0, 1); 
	fe.setTDAC(270, 0, 0); 

        fe.setEn(307, 0, 1);
	fe.setInjEn(307, 0, 1); 
	fe.setTDAC(307, 0, 0); 

      	fe.configurePixels(); 
    	while (!spec.isCmdEmpty()) {} //waits for the commands to be written into the registers

 for (int i=0; i<100; i++) {
    	std::cout << ">>> Injecting" << i+1 << std::endl;
        
        //Single Pulse Analog Injection (mode=0)
	fe.cal(0, 0, 0, 1, 0, 0); // Inject High -> Med
       	std::this_thread::sleep_for(std::chrono::milliseconds(1));
        fe.cal(0, 1, 0, 0, 0, 0); // Rearm cal_edge
	std::this_thread::sleep_for(std::chrono::milliseconds(1)); 

/*	//Double Pulse Analog Injection (mode=1)
	fe.cal(0, 1, 0, 20, 1, 10); // Inject High -> Med
       	std::this_thread::sleep_for(std::chrono::milliseconds(1));
        fe.cal(0, 1, 0, 0, 0, 0); // Rearm cal_edge
	std::this_thread::sleep_for(std::chrono::milliseconds(1));  */

       	std::this_thread::sleep_for(std::chrono::milliseconds(100));
}

       	std::this_thread::sleep_for(std::chrono::milliseconds(100));
       	

   while (!spec.isCmdEmpty()) {}
    
    //spec.setTrigEnable(0);
    spec.setRxEnable(0x0);
    return 0;
}
