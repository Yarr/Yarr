#include <cstdint>
#include <string>

#include "SpecCom.h"
#include "logging.h"
#include "LoggingConfig.h"
#include "storage.hpp"

auto logger = logging::make_log("switchLPM");

void printHelp() {
    std::cout << "./bin/switchLPM on/off \n -e <int>: enabled TX channels (decimal number from binary pattern starting from TX 0, for example 13 to switch on 1101, i.e. all TX channels apart from TX 1) \n -s <int> spec number \n -f <int> AC signal frequency in kHz (required to be > 80kHz for a square wave)" << std::endl;
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
    int enableTX = 15;
    int enable = 0;
    int delay = 16;

    if (strcmp(argv[1], "on") == 0){
	enable=1; 
    } else {
	enable=0;
    }

    while ((c = getopt(argc, argv, "hd:")) != -1) {
		switch (c) {
		case 'h':
		    printHelp();
		    return 0;
		case 'd':
		    delay = std::stoi(optarg);
		    break;
		default:
		    logger->critical("Invalid command line parameter(s) given!");
		    return -1;
	    }
    }

	
    SpecCom mySpec(specNum);

    //logger->info("Start writing to delay register  ...");
    //logger->info("Writing delay to Spec Card {}, with delay {}", specNum, delay );

    //mySpec.writeSingle(0x2 << 14 | 0x4, 1); 
    //mySpec.writeSingle(0x2 << 14 | 0x5, delay); 

    //uint32_t delay_en_readback = 0;
    //delay_en_readback=mySpec.readSingle(0x2<<14 | 0x4);
    //logger->info("Delay enable set to {} ", delay_en_readback);

    uint32_t delay_readback = 0;
    delay_readback=mySpec.readSingle(0x2<<14 | 0x5);
    logger->info("Delay value set to {} ", delay_readback);

    //std::this_thread::sleep_for(std::chrono::seconds(2));

    uint32_t delay_sync = 0;
    delay_sync=mySpec.readSingle(0x2<<14 | 0x1);
    logger->info("RX sync is {} ", delay_sync);

    logger->info("All done! ");


    return 0;
}
