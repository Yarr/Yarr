//////////////////////////////////////////////////
// A utility to read ADC of specific vmux 
// Based on read-register tool
//
// author: Emily Thompson
// e-mail: emily.anne.thompson@cern.ch
// Nov 2022
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
    std::cerr << " read-adc" << std::endl;
    std::cerr << std::endl;
    std::cerr << " Usage: " << argv[0] << " [options] MonitorV" << std::endl;
    std::cerr << " Options:" << std::endl;
    std::cerr << "   -r          Hardware controller JSON file path [required]" << std::endl;
    std::cerr << "   -c          Input connectivity JSON file path [required]" << std::endl;
    std::cerr << "   -i          Position of chip in connectivity file chips list, starting from 0 (default: all chips). Can take multiple chip positions, and results will always be returned in order of the chips in the connectivity file" << std::endl;
    std::cerr << "   -n          Chip name (if given will override use of chip index). Can take multiple chip names, and results will always be returned in order of the chips in the connectivity file." << std::endl;
    std::cerr << "   -s          Assume FE's have shared vmux, and set MonitorV register to this value (high-Z) on all FE's when not reading" << std::endl;
    std::cerr << "   -I          Measure current through vmux pad" << std::endl;
    std::cerr << "   -R          Return raw ADC count" << std::endl;
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
    std::vector<int> chip_idx;
    std::vector<std::string> chip_name;
    uint16_t monitorV = 0; 
    bool use_chip_name = false;
    bool meas_curr = false;
    bool return_count = false;
    int high_z = -1;
    bool shared_vmux = false;

    int c = 0;
    while (( c = getopt(argc, argv, "r:c:i:n:s:IRh")) != -1) {
        switch (c) {
            case 'r' :
                hw_controller_filename = optarg;
                break;
            case 'c' :
                connectivity_filename = optarg;
                break;
            case 'i' :
                try {
                    chip_idx.push_back(std::stoi(optarg));
                } catch (std::exception& e) {
                    std::cerr << "ERROR: Chip index must be an integer value (you provided: " << optarg << ")" << std::endl;
                    return 1;
                }
                break;
            case 'n' :
                chip_name.push_back(optarg);
                use_chip_name = true;
                break;
            case 's':
                shared_vmux = true;
                try {
                    high_z = std::stoi(optarg);
                } catch (std::exception& e) {
                    std::cerr << "ERROR: High-z register value must be an integer value (you provided: " << optarg << ")" << std::endl;
                    return 1;
                }
                break;
            case 'h' :
                print_usage(argv);
                return 0;
            case 'I' :
		meas_curr = true;
                break;
            case 'R' :
		return_count = true;
                break;
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
        monitorV = std::stoi(argv[optind++]);
    } catch (std::exception& e) {
        std::cerr << "ERROR: Invalid value provided for MonitorV: \"" << argv[optind++] << "\" (non-integer)" << std::endl;
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
   
    std::vector<std::pair<int, std::unique_ptr<FrontEnd>>> fes = {};
 
    auto chip_configs = jconn["chips"];
    size_t n_chips = chip_configs.size();

    // Record fe's and set to high-z if shared vmux
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
        
        if (shared_vmux){
            auto cfg = dynamic_cast<FrontEndCfg*>(fe.get());
            hw->setCmdEnable(cfg->getTxChannel()); 
            hw->setRxEnable(cfg->getRxChannel());
            hw->checkRxSync(); // Must be done per fe (Aurora link) and after setRxEnable().
            fe->readUpdateWriteNamedReg("MonitorV");
            fe->writeNamedRegister("MonitorV", high_z);
        }
        fes.push_back(std::make_pair(ichip, std::move(fe)));
    }

    for (auto& felist : fes) {
        int ichip = felist.first;
        auto& fe = felist.second;
        auto cfg = dynamic_cast<FrontEndCfg*>(fe.get());
        std::string current_chip_name = cfg->getName();
        if (!use_chip_name) {
            if ( chip_idx.size() == 0 || (std::find(chip_idx.begin(), chip_idx.end(), ichip)!= chip_idx.end()) ) {
                hw->setCmdEnable(cfg->getTxChannel()); 
                hw->setRxEnable(cfg->getRxChannel());
                hw->checkRxSync(); // Must be done per fe (Aurora link) and after setRxEnable().
                fe->confAdc(monitorV, meas_curr);
                fe->readUpdateWriteNamedReg("MonitoringDataAdc");
                uint16_t res = fe->readNamedRegister("MonitoringDataAdc");
		if (return_count) std::cout << res << std::endl;
                else{
                  std::pair<float, std::string> convertedAdc = cfg->convertAdc(res, meas_curr);
		  std::cout << convertedAdc.first << " " << convertedAdc.second << std::endl;
                }
            }
        } else {
            if (std::find(chip_name.begin(), chip_name.end(), current_chip_name) != chip_name.end()) {
                hw->setCmdEnable(cfg->getTxChannel()); 
                hw->setRxEnable(cfg->getRxChannel());
                hw->checkRxSync(); // Must be done per fe (Aurora link) and after setRxEnable().
                fe->confAdc(monitorV, meas_curr);
                fe->readUpdateWriteNamedReg("MonitoringDataAdc");
                uint16_t res = fe->readNamedRegister("MonitoringDataAdc");
		if (return_count) std::cout << res << std::endl;
                else{
                  std::pair<float, std::string> convertedAdc = cfg->convertAdc(res, meas_curr);
		  std::cout << convertedAdc.first << " " << convertedAdc.second << std::endl;
                }
            }
        }
    }

    std::cerr << "Done." << std::endl;

    return 0;
}
