#include <cstdint>
#include <iomanip>
#include <iostream>
#include <string>
#include <filesystem>
namespace fs = std::filesystem;
#include <unistd.h>

#include "logging.h"
#include "LoggingConfig.h"

#include "SpecCom.h"
#include "ScanHelper.h"

void print_help() {
    std::cout << "Usage: ./bin/specSoftReset [-h] [-r <hw_controller_file>] [-o <reset_option>] \n \n"
              << "Options:\n"
              << " -h                         Display help messages.\n"
              << " -r <hw_controller_file>    Specify hardware controller JSON path.\n"
              << " -o <reset_option>          Specify soft reset target.\n"
              << "                               -- 1 BRAM difference counter\n"
              << "                               -- 2 WSHEXP Core\n"
              << "                               -- 3 Tx Core\n"
              << "                               -- 4 Rx Core\n"
              << "                               -- 5 Rx Bridge\n"
              << "                               -- 6 Trigger Logic\n"
              << "                               -- 7 SPI \n"
              << "                               -- 8 CTRL register\n"
              << "                               -- 9 BRAM \n"
              << "                               -- 14 ALL Wishbone modules except ctrl_reg\n"
              << "                               -- 15 ALL Wishbone modules\n";
}

int main(int argc, char **argv) {
    // Setup logger with some defaults
    logging::setupLoggers(logging::defaultConfig());
    int c;
    
    int specNum = 0;
    uint32_t resetOption = 1;
    std::string hw_controller_filename = "";

    while ((c = getopt(argc, argv, "hr:o:")) != -1) {
       switch (c) {
           case 'h':
               print_help();
               return 0;
           case 'r':
               hw_controller_filename = optarg;
               break;
           case 'o':
               resetOption = std::atoi(optarg);
               break;
           default:
               print_help();
               return -1;
       }
    }

    fs::path hw_controller_path{hw_controller_filename};
    if(!fs::exists(hw_controller_path)) {
        std::cerr << "ERROR: Provided hw controller file (=" << hw_controller_filename << ") does not exist" << std::endl;
        return 1;
    }


    json jcontroller;
    jcontroller = ScanHelper::openJsonFile(hw_controller_filename);
    specNum=jcontroller["ctrlCfg"]["cfg"]["specNum"];

    SpecCom mySpec(specNum);

    if (resetOption == RESET_OPTION_BRAM_CNT) {
        std::cout << "Resetting BRAM Cnt ..." << std::endl;
        mySpec.writeSingle(SPEC_GREG | SPEC_GREG_SOFTRST, SOFTRST_BRAM_CNT);
    }

    if (resetOption == RESET_OPTION_EXCEPT_CTRL_REG) {
        std::cout << "Resetting ALL but Ctrl regs  ..." << std::endl;
        mySpec.writeSingle(SPEC_GREG | SPEC_GREG_SOFTRST, SOFTRST_EXCEPT_CTRL_REG);
    }
    
    if (resetOption == RESET_OPTION_ALL) {
        std::cout << "Resetting ALL  ..." << std::endl;
        mySpec.writeSingle(SPEC_GREG | SPEC_GREG_SOFTRST, SOFTRST_ALL);
    }

    if (resetOption == RESET_OPTION_WSHEXP_CORE) {
        std::cout << "Resetting Wishbone Cores  ..." << std::endl;
        mySpec.writeSingle(SPEC_GREG | SPEC_GREG_SOFTRST, SOFTRST_WSHEXP_CORE);
    }

    if (resetOption == RESET_OPTION_TX_CORE) {
        std::cout << "Resetting Tx Core  ..." << std::endl;
        mySpec.writeSingle(SPEC_GREG | SPEC_GREG_SOFTRST, SOFTRST_TX_CORE);
    }

    if (resetOption == RESET_OPTION_RX_CORE) {
        std::cout << "Resetting Rx Core  ..." << std::endl;
        mySpec.writeSingle(SPEC_GREG | SPEC_GREG_SOFTRST, SOFTRST_RX_CORE);
    }

    if (resetOption == RESET_OPTION_RX_BRIDGE) {
        std::cout << "Resetting Rx Bridge  ..." << std::endl;
        mySpec.writeSingle(SPEC_GREG | SPEC_GREG_SOFTRST, SOFTRST_RX_BRIDGE);
    }

    if (resetOption == RESET_OPTION_SPI) {
        std::cout << "Resetting SPI  ..." << std::endl;
        mySpec.writeSingle(SPEC_GREG | SPEC_GREG_SOFTRST, SOFTRST_SPI);
    }

    if (resetOption == RESET_OPTION_BRAM) {
        std::cout << "Resetting BRAM  ..." << std::endl;
        mySpec.writeSingle(SPEC_GREG | SPEC_GREG_SOFTRST, SOFTRST_BRAM);
    }

    if (resetOption == RESET_OPTION_CTRL_REG) {
        std::cout << "Resetting Ctrl Regs  ..." << std::endl;
        mySpec.writeSingle(SPEC_GREG | SPEC_GREG_SOFTRST, SOFTRST_CTRL_REG);
    }
    
    std::cout << "... done!" << std::endl;
    return 0;
}
