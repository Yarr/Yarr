//////////////////////////////////////////////////
// A utility to write a specific register to a
// a given front-end without running any other
// configuration.
//
// author: Daniel Antrim
// e-mail: daniel.joseph.antrim AT cern DOT ch
// July 2021
//////////////////////////////////////////////////

// std/stl
#include <iostream>
#include <string>
#include <sstream>
#include <getopt.h>
#include <memory>

#include <filesystem>
namespace fs = std::filesystem;
#include <memory> // unique_ptr

// YARR
#include "HwController.h"
#include "FrontEnd.h"
#include "AllChips.h"
#include "ScanHelper.h" // openJson
#include "LoggingConfig.h"

void print_usage(char* argv[]) {
    std::cerr << " write-register" << std::endl;
    std::cerr << std::endl;
    std::cerr << " Usage: " << argv[0] << " [options] register-name register-value" << std::endl;
    std::cerr << " Options:" << std::endl;
    std::cerr << "   -r          Hardware controller JSON file path [required]" << std::endl;
    std::cerr << "   -c          Input connectivity JSON file path [required]" << std::endl;
    std::cerr << "   -i          Position of chip in connectivity file chips list, starting from 0 (default: all chips)" << std::endl;
    std::cerr << "   -n          Chip name (if given will override use of chip index)" << std::endl;
    std::cerr << "   -h|--help   Print this help message and exit" << std::endl;
    std::cerr << std::endl;
}

auto logger = logging::make_log("rd53bEfuseCheck");

std::shared_ptr<FrontEnd> init_fe(std::unique_ptr<HwController>& hw, std::string config, int fe_num) {
    json jconn = ScanHelper::openJsonFile(config);
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
    fe->init(&*hw, FrontEndConnectivity(chip_config["tx"], chip_config["rx"]));
    auto chip_register_file_path = chip_config["config"];
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
    // Setup logger
    json loggerConfig;
    loggerConfig["pattern"] = "[%T:%e]%^[%=8l][%=15n][%t]:%$ %v";
    loggerConfig["log_config"][0]["name"] = "all";
    loggerConfig["log_config"][0]["level"] = "info";
    loggerConfig["outputDir"] = "";
    logging::setupLoggers(loggerConfig);

    std::string hw_controller_filename = "";
    std::string connectivity_filename = "";
    int chip_idx = -1;
    std::string chip_name = "";
    std::string register_name = "";
    uint32_t register_value = 0;
    bool use_chip_name = false;

    int c = 0;
    while (( c = getopt(argc, argv, "r:c:h")) != -1) {
        switch (c) {
            case 'r' :
                hw_controller_filename = optarg;
                break;
            case 'c' :
                connectivity_filename = optarg;
                break;
            case 'h' :
                print_usage(argv);
                return 0;
            default :
                std::cerr << "Invalid option '" << c << "' supplied, aborting" << std::endl;
                return 1;
        } // switch
    } // while

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

    int errors = 0;

    // open up the connectivity config to get the list of front-ends
    auto jconn = ScanHelper::openJsonFile(connectivity_filename);
    auto chip_configs = jconn["chips"];
    size_t n_chips = chip_configs.size();
    for (size_t ichip = 0; ichip < n_chips; ichip++) {
        fs::path chip_register_file_path{chip_configs[ichip]["config"]};
        if(!fs::exists(chip_register_file_path)) {
            std::cerr << "WARNING: Chip config for chip at index " << ichip << " in connectivity file does not exist, skipping" << std::endl;
            continue;
        }
        auto fe = init_fe(hw, connectivity_filename, ichip);
        
        auto feCfg = std::dynamic_pointer_cast<FrontEndCfg>(fe);
        hw->setCmdEnable(feCfg->getTxChannel());
        hw->setRxEnable(feCfg->getRxChannel());
        logger->info("Reading efuse of chip: {}", feCfg->getName()); 

        if (!fe->hasValidName()) {
            logger->error("Invalid chip name!");
            errors--;
        }
    }

    if (errors == 0) {
        logger->info("All IDs were read succesfully");
    } else {
        logger->error("There were errors reading back e-fuse IDs!");
    }

    std::cerr << "Done." << std::endl;

    return errors;
}
