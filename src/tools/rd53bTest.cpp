// ############################
// # Author: Timon Heim
// # Email: timon.heim at cern.ch
// # Project: Yarr
// # Description: RD53B testing
// # Date: June 2020
// ############################

#include <iostream>
#include <string>
#include <fstream>
#include <iomanip>

#include "storage.hpp"
#include "logging.h"
#include "LoggingConfig.h"

#include "ScanHelper.h"
#include "Bookkeeper.h"

#include "Rd53b.h"

auto logger = logging::make_log("rd53bTest");

void printHelp() {
    std::cout << "-c <string> : path to config" << std::endl;
}

int main (int argc, char *argv[]) {
    // Setup logger with some defaults
    std::string defaultLogPattern = "[%T:%e]%^[%=8l][%=15n]:%$ %v";
    spdlog::set_pattern(defaultLogPattern);
    json j; // empty
    j["pattern"] = defaultLogPattern;
    j["log_config"][0]["name"] = "all";
    j["log_config"][0]["level"] = "trace";
    logging::setupLoggers(j);
 
    logger->info("\033[1;31m###################\033[0m");
    logger->info("\033[1;31m# RD53B Test Tool #\033[0m");
    logger->info("\033[1;31m###################\033[0m");
    logger->info("Do not use unless you know what you are doing!");
    logger->info("Do not ask questions related to this tool, as you should know what you are doing!");

    logger->info("Parsing command line parameters ...");
    int c;
    std::string cfgFilePath = "configs/JohnDoe.json";
    std::string ctrlFilePath = "controller/specCfg.json";
    while ((c = getopt(argc, argv, "hc:r:")) != -1) {
        int count = 0;
        switch (c) {
            case 'h':
                printHelp();
                return 0;
                break;
            case 'r':
                ctrlFilePath = std::string(optarg);
                break;
            case 'c':
                cfgFilePath = std::string(optarg);
                break;
            default:
                spdlog::critical("No command line parameters given!");
                return -1;
        }
    }

    logger->info("Chip config file path  : {}", cfgFilePath);
    logger->info("Ctrl config file path  : {}", ctrlFilePath);

    logger->info("\033[1;31m#################\033[0m");
    logger->info("\033[1;31m# Init Hardware #\033[0m");
    logger->info("\033[1;31m#################\033[0m");

    logger->info("-> Opening controller config: {}", ctrlFilePath);

    std::unique_ptr<HwController> hwCtrl = nullptr;
    json ctrlCfg;
    try {
        ctrlCfg = ScanHelper::openJsonFile(ctrlFilePath);
        hwCtrl = ScanHelper::loadController(ctrlCfg);
    } catch (std::runtime_error &e) {
        logger->critical("Error opening or loading controller config: {}", e.what());
        return -1;
    }
    
    hwCtrl->setupMode();
    hwCtrl->setTrigEnable(0);

    Bookkeeper bookie(&*hwCtrl, &*hwCtrl);
    
    logger->info("\033[1;31m###################\033[0m");
    logger->info("\033[1;31m##  Chip Config  ##\033[0m");
    logger->info("\033[1;31m###################\033[0m");

    Rd53b rd53b;
    rd53b.init(&*hwCtrl, 0, 0);

    std::ifstream cfgFile(cfgFilePath);
    if (cfgFile) {
        // Load config
        logger->info("Loading config file: {}", cfgFilePath);
        json cfg;
        try {
            cfg = ScanHelper::openJsonFile(cfgFilePath);
        } catch (std::runtime_error &e) {
            logger->error("Error opening chip config: {}", e.what());
            throw(std::runtime_error("loadChips failure"));
        }
        rd53b.fromFileJson(cfg);
        cfgFile.close();
    } else {
        logger->warn("Config file not found, using default!");
        // Write default to file
        std::ofstream newCfgFile(cfgFilePath);
        json cfg;
        rd53b.toFileJson(cfg);
        newCfgFile << std::setw(4) << cfg;
        newCfgFile.close();
    }
 
    logger->info("Enable Tx");
    hwCtrl->setCmdEnable(0);

    logger->info("Configure chip ...");
    rd53b.configureInit();
    rd53b.configureGlobal();
    logger->info("... done! bye!");
    return 0;
}
