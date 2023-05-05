#include <cstdint>
#include <string>

#include "SpecCom.h"
#include "logging.h"
#include "LoggingConfig.h"
#include "storage.hpp"

auto logger = logging::make_log("delaySet");

void printHelp() {
    std::cout << "./bin/delaySet -s <int> spec number -l <int> target lane -d <int> delay -a automatic mode " << std::endl;
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
    int delay = 0;
    int target_lane= 0;
    bool automode = false;

    while ((c = getopt(argc, argv, "hs:l:d:a")) != -1) {
		switch (c) {
		case 'h':
		    printHelp();
		    return 0;
		case 's':
		    specNum = std::stoi(optarg);
		    break;
		case 'l':
		    target_lane = std::stoi(optarg);
		    break;	
		case 'd':
		    delay = std::stoi(optarg);
		    break;	
		case 'a':
		    automode = true;
		    break;	
		default:
		    logger->critical("Invalid command line parameter(s) given!");
		    return -1;
	    }
    }

	
    SpecCom mySpec(specNum);

    logger->info("Start writing to delay register  ...");
    logger->info("Writing delay to Spec Card {}, with delay {}", specNum, delay );


    if (automode) {
	// Disable manual delay control
	mySpec.writeSingle(0x2 << 14 | 0x6, 0); 
	return 0;
    }

	// Enable manual delay control
	mySpec.writeSingle(0x2 << 14 | 0x6, 15); 

    mySpec.writeSingle(0x2 << 14 | 0x4, target_lane); 
    mySpec.writeSingle(0x2 << 14 | 0x5, delay); 

    uint32_t delay_en_readback = 0;
    delay_en_readback=mySpec.readSingle(0x2<<14 | 0x4);
    uint32_t delay_readback = 0;
    delay_readback=mySpec.readSingle(0x2<<14 | 0x5);
    logger->info("Delay value set to {} on lane {}", delay_readback, delay_en_readback);

    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    uint32_t delay_sync = 0;
    delay_sync=mySpec.readSingle(0x2<<14 | 0x1);
    uint32_t rel_bit=(delay_sync >> target_lane) & 0x01; 
    logger->info("RX sync for lane {} is {:b} ", delay_en_readback,rel_bit);

    return 0;
}
