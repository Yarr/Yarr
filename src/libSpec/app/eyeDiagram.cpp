#include <cstdint>
#include <string>
#include <iomanip>

#include "SpecCom.h"
#include "logging.h"
#include "LoggingConfig.h"
#include "storage.hpp"

auto logger = logging::make_log("eyeDiagram");

void printHelp() {
    std::cout << "./bin/eyeDiagram -s <int> spec number -n <int> number of lanes " << std::endl;
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

    while ((c = getopt(argc, argv, "hs:n:")) != -1) {
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
	mySpec.writeSingle(0x2 << 14 | 0x6, 15); 

    std::string s = "";
    std::cout << "Delay " << "\t";
    for (uint32_t i = 0 ; i<32; i++) {
        std::cout << std::setw(2) << i << " ";
    }
    std::cout << std::endl;
	
    // Loop over lanes 
    for (uint32_t j = 0 ; j<n_lanes; j++) {
        std::cout << "Lane " << std::to_string(j) << "\t";

	    for (uint32_t i = 0 ; i<32; i++) {
		    mySpec.writeSingle(0x2 << 14 | 0x4, j); 
		    mySpec.writeSingle(0x2 << 14 | 0x5, i); 

		    uint32_t delay_en_readback = 0;
		    delay_en_readback=mySpec.readSingle(0x2<<14 | 0x4);
		    uint32_t delay_readback = 0;
		    delay_readback=mySpec.readSingle(0x2<<14 | 0x5);
		    logger->debug("Delay value set to {} on lane {}", delay_readback, delay_en_readback);

		    std::this_thread::sleep_for(std::chrono::milliseconds(300));

		    uint32_t delay_sync = 0;
		    delay_sync=mySpec.readSingle(0x2<<14 | 0x1);
			uint32_t rel_bit=(delay_sync >> j) & 0x01; 
		    logger->debug("RX sync for lane {} is {:b} ", delay_en_readback,rel_bit);
            std::cout << std::setw(2) << std::to_string(rel_bit) << " ";
    	}
	    std::cout << std::endl;
    }
	logger->info("All done! ");

	logger->info("Scan results: \n");
    std::cout << s << std::endl;
	file << s;
	// Disable manual delay control
	mySpec.writeSingle(0x2 << 14 | 0x6, 0); 

    return 0;
}
