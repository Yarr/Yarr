
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

#include "storage.hpp"
#include "logging.h"
#include "LoggingConfig.h"

#include "AllChips.h"
#include "FrontEnd.h"

auto logger = logging::make_log("cfgCreator");

void printHelp() {
    std::cout << "-t <string> : chip type" << std::endl;
    std::cout << "-s <string> : system type" << std::endl;
    std::cout << "-o <string> : output directory" << std::endl;
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
    std::string systemType = "SingleChip";
    std::string outputDir = "configs/";
    std::string chipName = "JohnDoe";
    while ((c = getopt(argc, argv, "ht:s:o:n:")) != -1) {
        int count = 0;
        switch (c) {
            case 'h':
                printHelp();
                return 0;
                break;
            case 't':
                chipType = optarg;
                break;
            case 's':
                systemType = optarg;
                break;
            case 'o':
                outputDir = optarg;
                if (outputDir.back() != '/')
                    outputDir = outputDir + "/";
                break;
            case 'n':
                chipName = optarg;
                break;
            default:
                spdlog::critical("No command line parameters given!");
                return -1;
        }
    }

    logger->info("Chip Type   : {}", chipType);
    logger->info("System Type : {}", systemType);
    logger->info("Chip Name   : {}", chipName);
    logger->info("Ouput directory  : {}", outputDir);

    // Create FE object
    std::unique_ptr<FrontEnd> fe = StdDict::getFrontEnd(chipType);
    if (!fe) {
        logger->error("Unknown chip type: {}", chipType);
        return -1;
    }

    FrontEndCfg *feCfg = dynamic_cast<FrontEndCfg*>(fe.get());

    feCfg->setName(chipName);

    // Write example config files
    try {
        feCfg->createExampleConfig(outputDir, systemType);
    } catch (const std::exception& e) {
        logger->error(e.what());
        logger->info("Failed to create config files.");
        return -1;
    }

    logger->info("... done! Success, bye!");
    return 0;
}
