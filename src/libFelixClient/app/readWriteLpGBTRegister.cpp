#include "FelixController.h"
#include "logging.h"
#include "LoggingConfig.h"
#include "ScanHelper.h"
#include "ScanOpts.h"

#include <iostream>

namespace {
    auto logger = logging::make_log("readWriteLpGBTRegister");

    void printHelp() {
        std::cout << "Usage: readWriteLpGBTRegister -c HW_CONFIG --regname REGNAME --regfield REGFIELD --regval [REGISTER_VALUE, optional] " << std::endl;
        std::cout << "  Read or write LpGBT registers via the FelixController" << std::endl;
        std::cout << " -h : Show this help." << std::endl;
        std::cout << "Examples:" << std::endl;
        std::cout << "* To read the LpGBT register EPRX00CONTROL - EPRX00TRACKMODE:" << std::endl;
        std::cout << "  bin/readWriteLpGBTRegister -c configs/controller/felix_client.json --regname \"EPRX00CONTROL\" --regfield \"EPRX00TRACKMODE\"" << std::endl;
        std::cout << "* To write 0 to the LpGBT register EPRX00CONTROL - EPRX00TRACKMODE:" << std::endl;
        std::cout << "  bin/readWriteLpGBTRegister -c configs/controller/felix_client.json --regname \"EPRX00CONTROL\" --regfield \"EPRX00TRACKMODE\" --regval 0" << std::endl;
    }
}

int main(int argc, char **argv) {
    std::string logCfg;

    int c = 0;
    std::string hw_controller_filename = "";

    std::string regname = "";
    std::string regfield = "";
    std::string regval_str = "";
    const char* const short_options = "r:n:f:v";
    const option long_options[] = {
        {"help",no_argument,nullptr,'h'},
        {"r",required_argument,nullptr,'r'},
        {"regname",required_argument,nullptr,'n'},
        {"regfield",required_argument,nullptr,'f'},
        {"regval",required_argument,nullptr,'v'},
        {nullptr, no_argument, nullptr, 0}
    };
    while ((c = getopt_long(argc, argv, short_options, long_options,nullptr)) != -1) {
        switch (c) {
        case 'h':
            printHelp();
            return 0;
		case 'r':
            hw_controller_filename = optarg;
            break;
        case 'n' :
            regname = optarg;
            break;
        case 'f' :
            regfield = optarg;
            break;
        case 'v' :
            regval_str = optarg;
            break;
	    default:
            logger->critical("Invalid command line parameter(s) given! See help message for instructions:");
            printHelp();
            return -1;
	    }
    }

    // Configure logger
    if (logCfg.empty()) { // default
        ScanOpts options;
        json jlog;
        jlog["pattern"] = options.defaultLogPattern;
        jlog["log_config"][0]["name"] = "all";
        jlog["log_config"][0]["level"] = "info";
        logging::setupLoggers(jlog);
    } 
    else{
        try{
            auto jlog = ScanHelper::openJsonFile(logCfg);
            logging::setupLoggers(jlog);
        } 
        catch (std::runtime_error &e){
            spdlog::error("Failed to load logger config: {}", e.what());
            return -1;
        }
    }

    // Configure controller
    json jctrl;
    try {
        jctrl = ScanHelper::openJsonFile(ctrlCfg);
        if (jctrl["ctrlCfg"]["type"] != "FelixClient") {
        logger->critical("The controller type is not FelixClient.");
        return -1;
        }
    } catch (std::runtime_error &e) {
        logger->critical("Cannot open controller config: {}", e.what());
        return -1;
    }

    auto hwCtrl = std::make_unique<FelixController>();

    try {
        hwCtrl->loadConfig(jctrl["ctrlCfg"]["cfg"]);
    } catch (std::runtime_error &e) {
        logger->error("Failed to load controller config: {}", e.what());
        return -1;
    }

    bool success = true;
    // read register
    if (regValue_str.empty()) {
        success = hwCtrl->readLpGBTRegister(regname, regfield, regval)
        if (!success){
            std::cerr << "ERROR reading register " << regname << ", " << regfield << std::endl;
        }
        else {
            std::cout << std::endl;
            std::cout << regName << " = 0x" << std::hex << regval << std::endl;
            std::cout << std::endl;
        }
    } 
    // write register
    else {
        bool success = hwCtrl->writeLpGBTRegister(regName, regfield, regval_str);
        if (!success){
            std::cerr << "ERROR reading register " << regname << ", " << regfield << std::endl;
        }
    }
    return 0;
}