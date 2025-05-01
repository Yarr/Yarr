#include "FelixController.h"
#include "logging.h"
#include "LoggingConfig.h"
#include "ScanHelper.h"
#include "ScanOpts.h"

#include <cstdint>
#include <fstream>
#include <string>
#include <iomanip>
#include <iostream>

namespace fs = std::filesystem;

namespace {
    auto logger = logging::make_log("readWriteLpGBTRegister");

    void printHelp() {
        std::cout << "Read or write LpGBT register by providing a register name" << std::endl;
        std::cout << "Read Usage: readLpGBTRegister -r HW_CONFIG -n \"REGNAME\" -R RX_FID -T TX_FID -d DEVICE_ADDRESS" << std::endl;
        std::cout << "Write Usage: readLpGBTRegister -r HW_CONFIG -n \"REGNAME\" -v REGVAL -R RX_FID -T TX_FID -d DEVICE_ADDRESS" << std::endl;
        std::cout << " -h : Show this help." << std::endl;
    }
}
  
int main(int argc, char **argv) {
    // Configure logger
    // default
    ScanOpts options;
    json jlog;
    jlog["pattern"] = options.defaultLogPattern;
    jlog["log_config"][0]["name"] = "all";
    jlog["log_config"][0]["level"] = "info";
    logging::setupLoggers(jlog);

    int c = 0;
    std::string hw_controller_filename = "";
    uint16_t devaddr = 0;
    uint64_t rx_fid = 0;
    uint64_t tx_fid = 0;
    std::string regname = "";
    uint8_t regval = 0;
    bool write = false;

    while ((c = getopt(argc, argv, "hr:n:v:R:T:d:")) != -1) {
        switch (c) {
        case 'h':
            printHelp();
            return 0;
		case 'r':
            hw_controller_filename = optarg;
            break;
        case 'n':
            regname = optarg;
            break;
        case 'v':
            regval = std::stoi(optarg);
            write = true;
            break;
        case 'R':
            rx_fid = std::stoull(optarg);
            break;
        case 'T':
            tx_fid = std::stoull(optarg);
            break;
        case 'd':
            devaddr = std::stoi(optarg);
            break;
		default:
            logger->error(" Invalid arguments provided");
            return -1;
	    }
    }

    // Configure controller
    json jctrl;
    try {
        jctrl = ScanHelper::openJsonFile(hw_controller_filename);
        if (jctrl["ctrlCfg"]["type"] != "FelixClient") {
            logger->error("The controller type is not FelixClient.");
            return -1;
        }
    } catch (std::runtime_error &e) {
        logger->error("Cannot open controller config: {}", e.what());
        return -1;
    }

    auto hwCtrl = std::make_unique<FelixController>();

    try {
        hwCtrl->loadConfig(jctrl["ctrlCfg"]["cfg"]);
    } catch (std::runtime_error &e) {
        logger->error("Failed to load controller config: {}",e.what());
        return -1;
    }
    auto* flxCtrlPtr = dynamic_cast<FelixController*>(hwCtrl.get());

    if (!write){
        hwCtrl->readLpGBTRegister((regname).c_str(), regval, devaddr, rx_fid, tx_fid);
        logger->info("Register with name {} read to have value 0x{:x}",regname, regval);
    }
    else {
        logger->info("Writing value 0x{:x} to register with name {}", regval, regname);
        hwCtrl->writeLpGBTRegister((regname).c_str(), regval, devaddr, rx_fid, tx_fid);
    }
    return 0;
}