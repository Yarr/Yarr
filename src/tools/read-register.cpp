//////////////////////////////////////////////////
// A utility to read a specific register from a
// a given front-end without running any other
// configuration. Based on write-register tool
//
// author: Timon Heim
// e-mail: timon.heim@cern.ch
// Oct 2022
//////////////////////////////////////////////////

// std/stl
#include <iostream>
#include <string>
#include <sstream>
#include <getopt.h>

#include <filesystem>
namespace fs = std::filesystem;
#include <memory> // unique_ptr

// YARR
#include "HwController.h"
#include "FrontEnd.h"
#include "AllChips.h"
#include "ScanHelper.h" // openJson
#include "Utils.h"

void print_usage(char* argv[]) {
    std::cerr << " read-register" << std::endl;
    std::cerr << std::endl;
    std::cerr << " Usage: " << argv[0] << " [options] register-name " << std::endl;
    std::cerr << " Options:" << std::endl;
    std::cerr << "   -r          Hardware controller JSON file path [required]" << std::endl;
    std::cerr << "   -c          Input connectivity JSON file path [required]" << std::endl;
    std::cerr << "   -i          Position of chip in connectivity file chips list, starting from 0 (default: all chips)" << std::endl;
    std::cerr << "   -n          Chip name (if given will override use of chip index)" << std::endl;
    std::cerr << "   -h|--help   Print this help message and exit" << std::endl;
    std::cerr << std::endl;
}

std::unique_ptr<FrontEnd> init_fe(std::unique_ptr<HwController>& hw, json &jconn, int fe_num) {
    std::string chip_type = jconn["chipType"];
    auto fe = StdDict::getFrontEnd(chip_type);
    auto cfg = dynamic_cast<FrontEndCfg*>(fe.get());
    auto chip_configs = jconn["chips"];
    if(fe_num >= chip_configs.size()) {
        std::stringstream e;
        e << "Invalid FE index (" << fe_num << ") for connectivity file with " << chip_configs.size() << " chips";
        throw std::runtime_error(e.str());
    }
    auto chip_config = chip_configs[fe_num];
    fe->init(&*hw, chip_config["tx"], chip_config["rx"]);
    auto chip_register_file_path = chip_config["__config_path__"];
    fs::path pconfig{chip_register_file_path};
    if(!fs::exists(pconfig)) {
        std::cerr << "WARNING: Chip config \"" << chip_register_file_path << "\" not found" << std::endl;
        fe.reset();
        return fe;
    }
    auto chip_register_json = ScanHelper::openJsonFile(chip_register_file_path);
    cfg->loadConfig(chip_register_json);
    return fe;
}

int main(int argc, char* argv[]) {
    std::string hw_controller_filename = "";
    std::string connectivity_filename = "";
    int chip_idx = -1;
    std::string chip_name = "";
    std::string register_name = "";
    uint32_t register_value = 0;
    bool use_chip_name = false;

    int c = 0;
    while (( c = getopt(argc, argv, "r:c:i:n:h")) != -1) {
        switch (c) {
            case 'r' :
                hw_controller_filename = optarg;
                break;
            case 'c' :
                connectivity_filename = optarg;
                break;
            case 'i' :
                try {
                    chip_idx = std::stoi(optarg);
                } catch (std::exception& e) {
                    std::cerr << "ERROR: Chip index must be an integer value (you provided: " << optarg << ")" << std::endl;
                    return 1;
                }
                break;
            case 'n' :
                chip_name = optarg;
                use_chip_name = true;
                break;
            case 'h' :
                print_usage(argv);
                return 0;
            default :
                std::cerr << "Invalid option '" << c << "' supplied, aborting" << std::endl;
                return 1;
        } // switch
    } // while

    if (optind > argc - 1) {
        std::cerr << "ERROR: Missing positional arguments" << std::endl;
        return 1;
    }

    register_name = argv[optind++];

    fs::path hw_controller_path{hw_controller_filename};
    if(!fs::exists(hw_controller_path)) {
        std::cerr << "ERROR: Provided hw controller file (=" << hw_controller_filename << ") does not exist" << std::endl;
        return 1;
    }

    fs::path connectivity_path{connectivity_filename};
    if(!fs::exists(connectivity_path)) {
        std::cerr << "ERROR: Provided connectivity file (=" << connectivity_filename << ") does not exist" << std::endl;
        return 1;
    }

    // instantiate the hw controller
    std::unique_ptr<HwController> hw;
    json jcontroller;
    try {
        jcontroller = ScanHelper::openJsonFile(hw_controller_filename);
        hw = ScanHelper::loadController(jcontroller);
    } catch (std::exception& e) {
        std::cerr << "ERROR: Unable to load controller from provided config, exception caught: " << e.what() << std::endl;
        return 1;
    }
    hw->setupMode();
    hw->setTrigEnable(0);
    hw->disableRx(); // needed?

    // open up the connectivity config to get the list of front-ends
    auto jconn = ScanHelper::openJsonFile(connectivity_filename);

    std::string chipType = ScanHelper::loadChipConfigs(jconn, false, Utils::dirFromPath(connectivity_filename));

    auto chip_configs = jconn["chips"];
    size_t n_chips = chip_configs.size();
    for (size_t ichip = 0; ichip < n_chips; ichip++) {
        if (chip_configs[ichip]["enable"] == 0)
	        continue;
        fs::path chip_register_file_path{chip_configs[ichip]["__config_path__"]};
        if(!fs::exists(chip_register_file_path)) {
            std::cerr << "WARNING: Chip config for chip at index " << ichip << " in connectivity file does not exist, skipping (" << chip_register_file_path << ")" << std::endl;
            continue;
        }

        auto fe = init_fe(hw, jconn, ichip);
        if(!fe) {
            std::cerr << "WARNING: Skipping chip at index " << ichip << " in connectivity file" << std::endl;
            continue;
        }
        auto cfg = dynamic_cast<FrontEndCfg*>(fe.get());
        std::string current_chip_name = cfg->getName();
        if (!use_chip_name) {
            if ( (chip_idx < 0) || (chip_idx == ichip) ) {
                hw->setCmdEnable(cfg->getTxChannel()); 
                hw->setRxEnable(cfg->getRxChannel());
                hw->checkRxSync(); // Must be done per fe (Aurora link) and after setRxEnable().
                fe->readUpdateWriteNamedReg(register_name);
                uint16_t res = fe->readNamedRegister(register_name);
                std::cout << res << std::endl;
            }
        } else {
            if (current_chip_name == chip_name) {
                hw->setCmdEnable(cfg->getTxChannel()); 
                hw->setRxEnable(cfg->getRxChannel());
                hw->checkRxSync(); // Must be done per fe (Aurora link) and after setRxEnable().
                fe->readUpdateWriteNamedReg(register_name);
                uint16_t res = fe->readNamedRegister(register_name);
                std::cout << res << std::endl;
            }
        }
    }

    std::cerr << "Done." << std::endl;

    return 0;
}
