#include "FelixController.h"
#include "logging.h"
#include "LoggingConfig.h"
#include "ScanHelper.h"
#include "ScanOpts.h"

#include <iostream>
#include <getopt.h>
#include <filesystem>
namespace fs = std::filesystem;

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
    std::string connectivity_filename = "";

    int regaddr = 0;

    const char* const short_options = "r:c:a:h";
    const option long_options[] = {
        {"help",no_argument,nullptr,'h'},
        {"r",required_argument,nullptr,'r'},
        {"c", required_argument, nullptr, 'c'},
        {"regaddr", required_argument, nullptr, 'a'},
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
        case 'c' :
            connectivity_filename = optarg;
            break;
        case 'a' :
            regaddr = std::stoi(optarg);
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

    std::string connectivity_pathname = "";
    // Configure connectivity, use to get rx
    if (connectivity_filename != ""){
        fs::path connectivity_path{connectivity_filename};
        if(!fs::exists(connectivity_path)) {
            std::cerr << "ERROR: Provided connectivity file (=" << connectivity_filename << ") does not exist" << std::endl;
        }
        connectivity_pathname = connectivity_filename.substr(0, connectivity_filename.find_last_of("/"));
    }
    auto jconn = ScanHelper::openJsonFile(connectivity_filename);

    auto chip_configs = jconn["chips"];
    int rx = chip_configs[3]["rx"];
    int tx = chip_configs[3]["tx"];


    // Configure controller
    json jctrl;
    try {
        jctrl = ScanHelper::openJsonFile(hw_controller_filename);
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
    auto* flxCtrlPtr = dynamic_cast<FelixController*>(hwCtrl.get());

    // read register
    uint8_t regval = 0;

    uint64_t rx_ic_fid = flxCtrlPtr->FelixRxCore::ic_fid_from_channel(rx);
    uint64_t tx_ic_fid = flxCtrlPtr->FelixTxCore::ic_fid_from_channel(tx);

    hwCtrl->readLpGBTRegister(regaddr, regval, rx_ic_fid, tx_ic_fid);
    std::cout << std::endl;
    std::cout << "register with address "  << regaddr << " has value " << " = 0x" << std::hex << regval << std::endl;
    std::cout << std::endl;

    return 0;
}