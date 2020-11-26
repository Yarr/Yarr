
// #################################
// # Author: Timon Heim
// # Email: timon.heim at cern.ch
// # Project: Yarr
// # Description: Simple tool to create config
// # Date: April 2020
// ################################

#include <iostream>
#include <string>
#include <fstream>
#include <iomanip>

#include "storage.hpp"
#include "logging.h"
#include "LoggingConfig.h"

#include "AllChips.h"
#include "FrontEnd.h"

auto logger = logging::make_log("cfgCreator");

void printHelp() {
    std::cout << "-t <string> : chip type" << std::endl;
    std::cout << "-o <string> : output file name and path" << std::endl;
    std::cout << "-n <string> : Chip Name/Serial number" << std::endl;
}

int main (int argc, char *argv[]) {
    // Setup logger with some defaults
    std::string defaultLogPattern = "[%T:%e]%^[%=8l][%=15n]:%$ %v";
    spdlog::set_pattern(defaultLogPattern);
    json j; // empty
    j["pattern"] = defaultLogPattern;
    j["log_config"][0]["name"] = "all";
    j["log_config"][0]["level"] = "info";
    logging::setupLoggers(j);
 
    logger->info("Starting config creator!");

    logger->info("Parsing command line parameters ...");
    int c;
    std::string chipType = "RD53A";
    std::string outFilePath = "configs/JohnDoe.json";
    std::string chipName = "JohnDoe";
    while ((c = getopt(argc, argv, "ht:o:n:")) != -1) {
        int count = 0;
        switch (c) {
            case 'h':
                printHelp();
                return 0;
                break;
            case 't':
                chipType = optarg;
                break;
            case 'o':
                outFilePath = optarg;
                break;
            case 'n':
                chipName = optarg;
                break;
            default:
                spdlog::critical("No command line parameters given!");
                return -1;
        }
    }

    logger->info("Chip Type  : {}", chipType);
    logger->info("Chip Name  : {}", chipName);
    logger->info("Ouput file : {}", outFilePath);
    
    // Create FE object
    std::unique_ptr<FrontEnd> fe = StdDict::getFrontEnd(chipType);
    FrontEndCfg *feCfg = dynamic_cast<FrontEndCfg*>(fe.get());

    feCfg->setName(chipName);

    json cfg;
    feCfg->toFileJson(cfg);

    logger->info("Writing default config to file ... ");
    std::ofstream outfile(outFilePath);
    outfile << std::setw(4) << cfg;
    outfile.close();
    logger->info("... done! Success, bye!");
    return 0;
}
