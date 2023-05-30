#include <cstdint>
#include <string>

#include "SpecCom.h"
#include "logging.h"
#include "LoggingConfig.h"
#include "storage.hpp"

auto logger = logging::make_log("eyeDiagram");

void printHelp() {
    std::cout << "./bin/eyeDiagram -s <int> spec number -n <int> number of lanes -o <int> port offset" << std::endl;
}

int main(int argc, char **argv) {
    // Setup logger with some defaults
    std::string defaultLogPattern = "[%T:%e]%^[%=8l][%=15n]:%$ %v";
    spdlog::set_pattern(defaultLogPattern);
    json j; // empty
    j["pattern"] = defaultLogPattern;
    j["log_config"][0]["name"] = "all";
    j["log_config"][0]["level"] = "info";
    logging::setupLoggers(j);
    
    // Init spec
    logger->info("Init spec");
    int c;	
    int specNum = 0;
    int delay = 16;
    int n_lanes= 4;
    int port_offset = 0;

    while ((c = getopt(argc, argv, "hs:n:o:")) != -1) {
		switch (c) {
		case 'h':
		    printHelp();
		    return 0;
		case 's':
		    specNum = std::stoi(optarg);
		    break;
		case 'n':
		    n_lanes = std::stoi(optarg);
		    break;	
		case 'o':
		    port_offset = std::stoi(optarg);
		    break;	
		default:
		    logger->critical("Invalid command line parameter(s) given!");
		    return -1;
	    }
    }

	
    SpecCom mySpec(specNum);

    logger->info("Start writing to delay register  ...");
    logger->info("Writing delay to Spec Card {}, with delay {}", specNum, delay );

    std::ofstream file;
    file.open("results.txt");

	// Enable manual delay control
	mySpec.writeSingle(0x2 << 14 | 0x6, 0xffff); 


	// Write error counter stop value and mode
	mySpec.writeSingle(0x2 << 14 | 0x8, 10000000); 
	mySpec.writeSingle(0x2 << 14 | 0x9, 0); 

    std::string s = "";
	// Loop over lanes 
    for (uint32_t j = 0 ; j<n_lanes; j++) {
		s+="Lane"+std::to_string(j)+"\t";
		// Loop over delays 
	    for (uint32_t i = 0 ; i<32; i++) {
		    mySpec.writeSingle(0x2 << 14 | 0x4, j+port_offset); 
		    mySpec.writeSingle(0x2 << 14 | 0x5, i); 

		    uint32_t delay_en_readback = 0;
		    delay_en_readback=mySpec.readSingle(0x2<<14 | 0x4);
		    uint32_t delay_readback = 0;
		    delay_readback=mySpec.readSingle(0x2<<14 | 0x5);
		    logger->info("Delay value set to {} on lane {}", delay_readback, delay_en_readback);

		    std::this_thread::sleep_for(std::chrono::milliseconds(500));

		    // Reset and restart error counter
		    mySpec.writeSingle(0x2 << 14 | 0xa, delay_en_readback); 
		    mySpec.writeSingle(0x2 << 14 | 0xb, 1); 
		    mySpec.writeSingle(0x2 << 14 | 0xb, 0); 

		    uint32_t errors = 0;
		   std::this_thread::sleep_for(std::chrono::milliseconds(1000));
		    errors=mySpec.readSingle(0x2<<14 | 0xb);
		   if ((errors>>31)&0x1) {
		    logger->info("RX errors for lane {} is {} ", delay_en_readback,errors&(0x7fffffff));
		   } else {
		    logger->info("Not enough data for RX errors for lane {}", delay_en_readback);
		   }
		s+=std::to_string(errors&0x7fffffff)+" ";
    	}
	s+="\n";
    }
	logger->info("All done! ");

	logger->info("Scan results: \n");
    std::cout << s << std::endl;
	file << s;
	// Disable manual delay control
	mySpec.writeSingle(0x2 << 14 | 0x6, 0); 

    return 0;
}
