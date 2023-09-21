#include <cstdint>
#include <iomanip>
#include <string>
#include <filesystem>
namespace fs = std::filesystem;

#include "SpecCom.h"
#include "ScanHelper.h"

#define SPEC_GREG_ADDR (0x7 << 14)
#define SPEC_GREG_SOFTRST 0xF

#define RESET_OPTION_BRAM_CNT 0x1
#define RESET_OPTION_WSHEXP_CORE 0x2
#define RESET_OPTION_TX_CORE 0x3
#define RESET_OPTION_RX_CORE 0x4
#define RESET_OPTION_RX_BRIDGE 0x5
#define RESET_OPTION_TRIG_LOGIC 0x6
#define RESET_OPTION_SPI 0x7
#define RESET_OPTION_CTRL_REG 0x8
#define RESET_OPTION_BRAM 0x9
#define RESET_OPTION_EXCEPT_CTRL_REG 0xE
#define RESET_OPTION_ALL 0xF

#define SOFTRST_WSHEXP_CORE 0x00000001
#define SOFTRST_TX_CORE 0x00000002
#define SOFTRST_RX_CORE 0x00000004
#define SOFTRST_RX_BRIDGE 0x00000008
#define SOFTRST_TRIGGER_LOGIC 0x00000020
#define SOFTRST_SPI 0x00000040
#define SOFTRST_CTRL_REG 0x00000080
#define SOFTRST_BRAM 0x00000100
#define SOFTRST_BRAM_CNT 0x00000200
#define SOFTRST_ALL 0x000003FF
#define SOFTRST_EXCEPT_CTRL_REG 0x0000037F


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
        mySpec.writeSingle(SPEC_GREG_ADDR | SPEC_GREG_SOFTRST, SOFTRST_BRAM_CNT);
    }

    if (resetOption == RESET_OPTION_EXCEPT_CTRL_REG) {
        mySpec.writeSingle(SPEC_GREG_ADDR | SPEC_GREG_SOFTRST, SOFTRST_EXCEPT_CTRL_REG);
    }
    
    if (resetOption == RESET_OPTION_ALL) {
        mySpec.writeSingle(SPEC_GREG_ADDR | SPEC_GREG_SOFTRST, SOFTRST_ALL);
    }

    if (resetOption == RESET_OPTION_WSHEXP_CORE) {
        mySpec.writeSingle(SPEC_GREG_ADDR | SPEC_GREG_SOFTRST, SOFTRST_WSHEXP_CORE);
    }

    if (resetOption == RESET_OPTION_TX_CORE) {
        mySpec.writeSingle(SPEC_GREG_ADDR | SPEC_GREG_SOFTRST, SOFTRST_TX_CORE);
    }

    if (resetOption == RESET_OPTION_RX_CORE) {
        mySpec.writeSingle(SPEC_GREG_ADDR | SPEC_GREG_SOFTRST, SOFTRST_RX_CORE);
    }

    if (resetOption == RESET_OPTION_RX_BRIDGE) {
        mySpec.writeSingle(SPEC_GREG_ADDR | SPEC_GREG_SOFTRST, SOFTRST_RX_BRIDGE);
    }

    if (resetOption == RESET_OPTION_SPI) {
        mySpec.writeSingle(SPEC_GREG_ADDR | SPEC_GREG_SOFTRST, SOFTRST_SPI);
    }

    if (resetOption == RESET_OPTION_BRAM) {
        mySpec.writeSingle(SPEC_GREG_ADDR | SPEC_GREG_SOFTRST, SOFTRST_BRAM);
    }

    if (resetOption == RESET_OPTION_CTRL_REG) {
        mySpec.writeSingle(SPEC_GREG_ADDR | SPEC_GREG_SOFTRST, SOFTRST_CTRL_REG);
    }

    return 0;
}
