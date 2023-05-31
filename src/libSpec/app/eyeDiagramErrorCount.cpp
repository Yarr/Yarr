#include <cstdint>
#include <string>
#include <iomanip>

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
    uint32_t test_size = 10e6;
	mySpec.writeSingle(0x2 << 14 | 0x8, test_size); 
	mySpec.writeSingle(0x2 << 14 | 0x9, 0); 

    std::vector<std::vector<double> > resultVec;
    resultVec.resize(n_lanes);
    for (uint32_t j=0; j<n_lanes; j++)
        resultVec[j].resize(32);

    for (uint32_t i = 0; i<32; i++) {
        // Write delay
        std::cout << std::setw(5) << i << " | ";
        for (uint32_t j = 0 ; j<n_lanes; j++) {
            mySpec.writeSingle(0x2 << 14 | 0x4, j+port_offset); 
            mySpec.writeSingle(0x2 << 14 | 0x5, i); 
        }
    
        // Wait for sync
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
        
        // Reset and restart error counter
        mySpec.writeSingle(0x2 << 14 | 0xb, 1); 
        mySpec.writeSingle(0x2 << 14 | 0xb, 0); 
        
        std::this_thread::sleep_for(std::chrono::milliseconds(2000));
        
        for (uint32_t j = 0 ; j<n_lanes; j++) {
		    mySpec.writeSingle(0x2 << 14 | 0xa, j+port_offset); 
		    uint32_t errors = 0;
		    errors = mySpec.readSingle(0x2<<14 | 0xb);
            double error_rate = 0;
            if (((errors>>31)&0x1)) {
                error_rate = (0x7FFFFFFF & errors);
                error_rate = (test_size-(test_size/49))/(test_size-error_rate);
                error_rate = (error_rate - 1) *1e6;
            }
            resultVec[j][i] = error_rate;
            std::cout << std::setw(7) << (0x7FFFFFFF & errors) << " | ";
        }
        std::cout << std::endl;
    }
	logger->info("All done! ");

	logger->info("Scan results: \n");
    for (uint32_t i = 0 ; i<32; i++) {
        // Write delay
        std::cout << std::setw(5) << i << "| ";
        for (uint32_t j = 0 ; j<n_lanes; j++) {
            std::cout << std::setw(7) << resultVec[j][i] << " | ";
        }
        std::cout << std::endl;
    }
    
    return 0;
}
