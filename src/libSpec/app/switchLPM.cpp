#include <cstdint>
#include <string>

#include "SpecCom.h"
#include "logging.h"
#include "LoggingConfig.h"
#include "storage.hpp"

auto logger = logging::make_log("switchLPM");

void printHelp() {
    std::cout << "./bin/switchLPM on/off -e <int>: enabled TX channels (from binary number, e.g. for 1111 provide 15) -s <int> spec number -f <int> AC signal frequency in kHz" << std::endl;
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
    int frequency = 100;

    if (strcmp(argv[1], "on") == 0){
	enable=1; 
    } else {
	enable=0;
    }

    while ((c = getopt(argc, argv, "he:f:s:")) != -1) {
		switch (c) {
		case 'h':
		    printHelp();
		    return 0;
		case 'e':
		    enableTX = std::stoi(optarg);
		    break;
		case 's':
		    specNum = std::stoi(optarg);
		    break;
		case 'f':
		    frequency = std::stoi(optarg);
		    break;
		default:
		    logger->critical("Invalid command line parameter(s) given!");
		    return -1;
	    }
    }

    SpecCom mySpec(specNum);

    logger->info("Start writing to low power enable register  ...");
    logger->info("Enabling LPM on TX channels {} of Spec Card {}, using Frequency {} kHz", enable*enableTX, specNum, frequency );

    mySpec.writeSingle(0x7<<14 | 0x0, enable*enableTX); 
    int count=0;
    count=160000/(2*frequency);

    mySpec.writeSingle(0x7<<14 | 0x1, count); 

    uint32_t ctrl_reg_0 = 0;
    ctrl_reg_0=mySpec.readSingle(0x7<<14 | 0x0);
    logger->info("Low power mode control register set to {} ", ctrl_reg_0);

    uint32_t ctrl_reg_1 = 0;
    ctrl_reg_1=mySpec.readSingle(0x7<<14 | 0x1);
    logger->info("Frequency control register set to {} counts ", ctrl_reg_1);

    logger->info("All done! ");


    return 0;
}
