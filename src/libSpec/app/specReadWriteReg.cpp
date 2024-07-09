#include <cstdint>
#include <iomanip>
#include <string>
#include <filesystem>
namespace fs = std::filesystem;

#include "SpecCom.h"
#include "ScanHelper.h"
#include "SpecTxCore.h"


void print_help() {
    std::cout << "Usage: ./bin/specReadWriteReg [-h] [-r <hw_controller_file>] [-w] [-o <register_option>]\n \n"
              << "Options:\n"
              << " -h                         Display help messages.\n"
              << " -r <hw_controller_file>    Specify hardware controller JSON path.\n"
              << " -w                         Whether to read (default) or write (-w)\n"
              << " -o <reg_addr>              Specify HW register address.\n"
              << " -b <base_addr>             Specify HW base register address (default TX_ADDR = " << TX_ADDR << ", consider TRIG_LOGIC_ADR = " << TRIG_LOGIC_ADR << ")\n"
              << " -v <value>                 Specify HW register write value (default 1).\n"
              << "                               -- TX_FIFO 0x0 = " << 0x0 << "\n"
              << "                               -- TX_ENABLE 0x1 = " << 0x1 << "\n"
              << "                               -- TX_EMPTY 0x2 = " << 0x2 << "\n"
              << "                               -- TRIG_EN 0x3 = " << 0x3 << "\n"
              << "                               -- TRIG_DONE 0x4 = " << 0x4 << "\n"
              << "                               -- TRIG_CONF 0x5 = " << 0x5 << "\n"
              << "                               -- TRIG_FREQ 0x6 = " << 0x6 << "\n"
              << "                               -- TRIG_TIME 0x7 = " << 0x7 << "\n"
              << "                               -- TRIG_COUNT 0x9 = " << 0x9 << "\n"
              << "                               -- TRIG_WORD_LENGTH 0xA = " << 0xA << "\n"
              << "                               -- TRIG_WORD 0xB = " << 0xB << "\n"
              << "                               -- TRIG_WORD_POINTER 0xC = " << 0xC << "\n"
              << "                               -- TX_PULSE_WORD 0xD = " << 0xD << "\n"
              << "                               -- TX_PULSE_INTERVAL 0xE = " << 0xE << "\n"
              << "                               -- TRIG_ABORT 0xF = " << 0xF << "\n"
              << "                               -- TRIG_IN_CNT 0xF = " << 0xF << "\n"
              << "                               -- TX_POLARITY 0x10 = " << 0x10 << "\n"
              << "                               -- TX_SYNC_WORD 0x11 = " << 0x11 << "\n"
              << "                               -- TX_SYNC_INTERVAL 0x12 = " << 0x12 << "\n"
              << "                               -- TX_IDLE_WORD 0x13 = " << 0x13 << "\n"
              << "                               -- TRIG_EXTEND_INTERVAL 0x14 = " << 0x14 << "\n"
              << "                               -- TRIG_ENCODER_ENABLE 0x15 = " << 0x15 << "\n";
}

int main(int argc, char **argv) {
    int c;
    int specNum = 0;
    uint32_t regOption = 0;
    uint32_t regValue = 0;
    uint32_t baseAddr = TX_ADDR;
    std::string hw_controller_filename = "";
    bool read=true;

    while ((c = getopt(argc, argv, "hwr:o:v:b:")) != -1) {
        switch (c) {
            case 'h':
                print_help();
                return 0;
            case 'r':
                hw_controller_filename = optarg;
                break;
            case 'w':
                read=false;
                break;
            case 'o':
                regOption = std::atoi(optarg);
                break;
            case 'v':
                regValue = std::atoi(optarg);
                break;
            case 'b': 
                baseAddr = std::atoi(optarg);
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

    if(read) {
        uint32_t rValue = mySpec.readSingle(baseAddr | regOption);
        std::cout << "Register " << regOption << " value: " << rValue << std::endl;
    }
    else {
        mySpec.writeSingle(baseAddr | regOption, regValue);
        std::cout << "Wrote register " << regOption << " with value: " << regValue << std::endl;
    }
    return 0;
}
