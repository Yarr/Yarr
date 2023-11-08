//////////////////////////////////////////////////
// A utility to perform a global reset of all
// front-ends on the same TX segment
//
// author: Daniel Antrim
// e-mail: daniel.joseph.antrim AT cern DOT ch
// March 2022
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
#include "Bookkeeper.h"

void print_usage(char* argv[]) {
    std::cerr << " reset-all" << std::endl;
    std::cerr << std::endl;
    std::cerr << " Send a global reset to all front-ends on the same control segment (tx line)" << std::endl;
    std::cerr << " Usage: " << argv[0] << " [options]" << std::endl;
    std::cerr << " Options:" << std::endl;
    std::cerr << "   -r          Hardware controller JSON file path [required]" << std::endl;
    std::cerr << "   -c          Input connectivity JSON file path [required]" << std::endl;
    std::cerr << "   -h|--help   Print this help message and exit" << std::endl;
    std::cerr << std::endl;
}

int main(int argc, char* argv[]) {
    std::string hw_controller_filename = "";
    std::string connectivity_filename = "";

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

    if(auto p = fs::path(hw_controller_filename); !fs::exists(p)) {
        std::cerr << "ERROR: Provided hw controller file (=" << hw_controller_filename << ") does not exist" << std::endl;
        return 1;
    }

    if(auto p = fs::path(connectivity_filename); !fs::exists(p)) {
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

    json jconnectivity;
    try {
        jconnectivity = ScanHelper::openJsonFile(connectivity_filename);
        if(!jconnectivity.contains("chipType")) {
            std::cerr << "ERROR: Connectivity file is missing expected \"chipType\" field" << std::endl;
            return 1;
        }
    } catch (std::exception& e) {
        std::cerr << "ERROR: Unable to load connectivity from provided config, exception caught: " << e.what() << std::endl;
        return 1;
    }
    std::string chipType = jconnectivity["chipType"];

    hw->setupMode();
    hw->setTrigEnable(0);
    hw->disableRx(); // needed?

    // set up the global FE and perform the reset as in scanConsole
    std::unique_ptr<Bookkeeper> bookie = std::make_unique<Bookkeeper>(&*hw, &*hw);
    bookie->initGlobalFe(StdDict::getFrontEnd(chipType).release());
    bookie->getGlobalFe()->makeGlobal();
    bookie->getGlobalFe()->init(&*hw, 0, 0);
    hw->setCmdEnable(bookie->getTxMaskUnique());
    bookie->getGlobalFe()->resetAllHard();

    return 0;
}
