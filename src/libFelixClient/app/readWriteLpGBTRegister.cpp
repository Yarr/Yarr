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
    void printHelp() {
        std::cout << "Read or write LpGBT register by either providing a register address or register name" << std::endl;
        std::cout << "Read by Name Usage: readLpGBTRegister -r HW_CONFIG -n \"REGNAME\" -R RX_FID -T TX_FID -d DEVICE_ADDRESS" << std::endl;
        std::cout << "Read by Address Usage: readLpGBTRegister -r HW_CONFIG -a REGADDR -R RX_FID -T TX_FID -d DEVICE_ADDRESS" << std::endl;
        std::cout << "Write by Name Usage: readLpGBTRegister -r HW_CONFIG -n \"REGNAME\" -v REGVAL -R RX_FID -T TX_FID -d DEVICE_ADDRESS" << std::endl;
        std::cout << "Write by Address Usage: readLpGBTRegister -r HW_CONFIG -a REGADDR -v REGVAL -R RX_FID -T TX_FID -d DEVICE_ADDRESS" << std::endl;
        std::cout << " -h : Show this help." << std::endl;
    }
}

int main(int argc, char **argv) {
    int c = 0;
    std::string hw_controller_filename = "";
    uint16_t devaddr = 0;
    uint64_t rx_fid = 0;
    uint64_t tx_fid = 0;
    std::string regname = "";
    uint16_t regaddr = 0;
    uint8_t regval = 0;
    bool write = false;

    while ((c = getopt(argc, argv, "hr:n:a:v:R:T:d:")) != -1) {
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
        case 'a':
            regaddr = std::stoi(optarg);
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
            std::cerr << " Invalid arguments provided" << std::endl;
            return -1;
	    }
    }
    // Configure controller
    json jctrl;
    try {
        jctrl = ScanHelper::openJsonFile(hw_controller_filename);
        if (jctrl["ctrlCfg"]["type"] != "FelixClient") {
            std::cerr << "The controller type is not FelixClient." << std::endl;
            return -1;
        }
    } catch (std::runtime_error &e) {
        std::cerr << "Cannot open controller config: " << e.what()  << std::endl;
        return -1;
    }

    auto hwCtrl = std::make_unique<FelixController>();

    try {
        hwCtrl->loadConfig(jctrl["ctrlCfg"]["cfg"]);
    } catch (std::runtime_error &e) {
        std::cerr << "Failed to load controller config: " << e.what() << std::endl;
        return -1;
    }
    auto* flxCtrlPtr = dynamic_cast<FelixController*>(hwCtrl.get());


    if (!write){
        if (regname == ""){
            hwCtrl->readLpGBTRegister(regaddr, regval, devaddr, rx_fid, tx_fid);
            std::cout << "Register with address " << regaddr << " read to have value 0x" << std::hex << static_cast<int>(regval) << std::endl;
        }
        else {
            hwCtrl->readLpGBTRegister((regname).c_str(), regval, devaddr, rx_fid, tx_fid);
            std::cout << "Register with name " << regname << " read to have value 0x" << std::hex << static_cast<int>(regval) << std::endl;
        }
    }
    else {
        if (regname == ""){
            std::cout << "Writing value " << std::hex << static_cast<int>(regval) << " to register with address " << regaddr << std::endl;
            hwCtrl->writeLpGBTRegister(regaddr, regval, devaddr, rx_fid, tx_fid);
        }
        else {
            std::cout << "Writing value " << std::hex << static_cast<int>(regval) << " to register with name " << regname << std::endl;
            hwCtrl->writeLpGBTRegister((regname).c_str(), regval, devaddr, rx_fid, tx_fid);
        }
    }
    return 0;
}