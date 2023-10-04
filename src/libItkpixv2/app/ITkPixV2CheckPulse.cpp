//////////////////////////////////////////////////
// Script specific to running data merging tests 
// on two SCCs
//
// Sends a global pulse of a particular length to 
// specific pulse configuration bits as broadcast 
// and checks communication status for both chips
//
// Author: Maria Mironova (maria.mironova@cern.ch)
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
#include "Itkpixv2.h"
#include "AllChips.h"
#include "ScanHelper.h" // openJson
#include "Utils.h"

#include "logging.h"
#include "LoggingConfig.h"

auto logger = logging::make_log("ITkPixV2CheckPulse");


void print_usage(char* argv[]) {
    std::cerr << " send-global-pulse" << std::endl;
    std::cerr << std::endl;
    std::cerr << " Usage: " << argv[0] << " [options] pulse-conf pulse-width" << std::endl;
    std::cerr << " Options:" << std::endl;
    std::cerr << "   -r          Hardware controller JSON file path [required]" << std::endl;
    std::cerr << "   -c          Input connectivity JSON file path [required]" << std::endl;
    std::cerr << "   -i          Position of chip in connectivity file chips list, starting from 0 (default: all chips)" << std::endl;
    std::cerr << "   -h|--help   Print this help message and exit" << std::endl;
    std::cerr << std::endl;
}

std::unique_ptr<FrontEnd> init_fe1(std::unique_ptr<HwController>& hw, json &jconn, int fe_num) {
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
    auto chip_register_file_path1 = chip_config["__config_path__"];
    fs::path pconfig{chip_register_file_path1};
    if(!fs::exists(pconfig)) {
        std::cerr << "WARNING: Chip config \"" << chip_register_file_path1 << "\" not found" << std::endl;
        fe.reset();
        return fe;
    }
    auto chip_register_json = ScanHelper::openJsonFile(chip_register_file_path1);
    cfg->loadConfig(chip_register_json);
    return fe;
}

std::unique_ptr<FrontEnd> init_fe2(std::unique_ptr<HwController>& hw, json &jconn, int fe_num) {
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
    auto chip_register_file_path2 = chip_config["__config_path__"];
    fs::path pconfig{chip_register_file_path2};
    if(!fs::exists(pconfig)) {
        std::cerr << "WARNING: Chip config \"" << chip_register_file_path2 << "\" not found" << std::endl;
        fe.reset();
        return fe;
    }
    auto chip_register_json = ScanHelper::openJsonFile(chip_register_file_path2);
    cfg->loadConfig(chip_register_json);
    return fe;
}
int main(int argc, char* argv[]) {
    std::string hw_controller_filename = "";
    std::string connectivity_filename = "";
    int chip_idx = -1;
    std::string chip_name = "";
    uint32_t pulse_conf = 0;
    uint32_t pulse_width = 0;
    bool use_chip_name = false;
    int n_idle = 0;

    int c = 0;
    while (( c = getopt(argc, argv, "r:c:i:h")) != -1) {
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

    try {
        pulse_width = std::stoi(argv[optind++]);
    } catch (std::exception& e) {
        std::cerr << "ERROR: Invalid value provided for pulse width (non-integer)" << std::endl;
        return 1;
    }

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
    
    fs::path chip_register_file_path1{chip_configs[0]["__config_path__"]};
    auto fe1 = init_fe1(hw, jconn, 0);
    fs::path chip_register_file_path2{chip_configs[1]["__config_path__"]};
    auto fe2 = init_fe2(hw, jconn, 1);
    //fs::path chip_register_file_path{chip_configs[1]["__config_path__"]};

    auto cfg1 = dynamic_cast<FrontEndCfg*>(fe1.get());
    auto cfg2 = dynamic_cast<FrontEndCfg*>(fe2.get());

    std::string current_chip_name1 = cfg1->getName();
    std::string current_chip_name2 = cfg2->getName();

    hw->setCmdEnable(0); 
    hw->setRxEnable(0);
    hw->checkRxSync(); // Must be done per fe (Aurora link) and after setRxEnable().


    int fail=0;
    int fail_1=0;
    int fail_2=0;

    fe1->writeNamedRegister("GlobalPulseConf", 16);
    fe1->writeNamedRegister("GlobalPulseWidth", pulse_width);
    fe2->writeNamedRegister("GlobalPulseConf", 8);
    fe2->writeNamedRegister("GlobalPulseWidth", pulse_width);


    std::this_thread::sleep_for(std::chrono::microseconds(100000));
    for (int j=0; j<1000; j++){
        while(!hw->isCmdEmpty()){;}

        hw->flushBuffer();

        //fe1->writeNamedRegister("ClkDataMergeEn", 1);     

        std::this_thread::sleep_for(std::chrono::microseconds(1000));


        //std::cout << "Sending Pulse Primary " << dynamic_cast<Itkpixv2Cfg*>(cfg1)->getChipId() << std::endl;
        dynamic_cast<Itkpixv2&>(*fe1).sendGlobalPulse(16);

        hw->writeFifo(0xAAAAAAAA);
        hw->writeFifo(0xAAAAAAAA);

        while(!hw->isCmdEmpty()){;}

        std::this_thread::sleep_for(std::chrono::microseconds(20));

        hw->flushBuffer();

        //std::this_thread::sleep_for(std::chrono::microseconds(1000));
         if (fe1->checkCom() != 1) {
            std::cout << "Trying again Primary" << std::endl;

            if (fe1->checkCom() != 1) {
                std::cout << "Failed again" << std::endl;
                fail_1++;
                
            } else {
                std::cout << "Fine now" << std::endl;
            }
         } 
 
         if (fe2->checkCom() != 1) {
            std::cout << "Trying again Secondary" << std::endl;                
            //break;

            if (fe2->checkCom() != 1) {
                std::cout << "Failed again" << std::endl;
                fail_2++;
                //std::cout << j << "Secondary Can't establish communication, aborting!" << std::endl;
            } else {
                std::cout << "Fine now" << std::endl;
            }
         } 

        std::this_thread::sleep_for(std::chrono::microseconds(100));
        std::cout << j << "  " << fail_1 << "   " << fail_2 << std::endl;


    }
    std::cout << fail_1 << "   " << fail_2 << std::endl;
    
    std::cerr << "Done." << std::endl;

    return 0;
}