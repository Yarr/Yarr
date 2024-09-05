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
              << "Address Map:\n"
              << "  TX_ADDR: 0x1 << 14 = " << (0x1 << 14) << "\n"
              << "     -- TX_FIFO 0x0 = " << 0x0 << "\n"
              << "     -- TX_ENABLE 0x1 = " << 0x1 << "\n"
              << "     -- TX_EMPTY 0x2 = " << 0x2 << "\n"
              << "     -- TRIG_EN 0x3 = " << 0x3 << "\n"
              << "     -- TRIG_DONE 0x4 = " << 0x4 << "\n"
              << "     -- TRIG_CONF 0x5 = " << 0x5 << "\n"
              << "     -- TRIG_FREQ 0x6 = " << 0x6 << "\n"
              << "     -- TRIG_TIME 0x7 = " << 0x7 << "\n"
              << "     -- TRIG_COUNT 0x9 = " << 0x9 << "\n"
              << "     -- TRIG_WORD_LENGTH 0xA = " << 0xA << "\n"
              << "     -- TRIG_WORD 0xB = " << 0xB << "\n"
              << "     -- TRIG_WORD_POINTER 0xC = " << 0xC << "\n"
              << "     -- TX_PULSE_WORD 0xD = " << 0xD << "\n"
              << "     -- TX_PULSE_INTERVAL 0xE = " << 0xE << "\n"
              << "     -- TRIG_ABORT 0xF = " << 0xF << "\n"
              << "     -- TRIG_IN_CNT 0xF = " << 0xF << "\n"
              << "     -- TX_POLARITY 0x10 = " << 0x10 << "\n"
              << "     -- TX_SYNC_WORD 0x11 = " << 0x11 << "\n"
              << "     -- TX_SYNC_INTERVAL 0x12 = " << 0x12 << "\n"
              << "     -- TX_IDLE_WORD 0x13 = " << 0x13 << "\n"
              << "     -- TRIG_EXTEND_INTERVAL 0x14 = " << 0x14 << "\n"
              << "     -- TRIG_ENCODER_ENABLE 0x15 = " << 0x15 << "\n"
              << "     -- TRIG_CODE_READY_COUNTER 0x16 = " << 0x16 << "\n"
              << "  RX_ADDR: 0x2 << 14 = " << (0x2 << 14) << "\n"
              << "     -- RX_ENABLE: 0x0 = " << 0x0 << "\n"
              << "     -- RX_STATUS: 0x1 = " << 0x1 << "\n"
              << "     -- RX_POLARITY: 0x2 = " << 0x2 << "\n"
              << "     -- RX_ACTIVE_LANES: 0x3 = " << 0x3 << "\n"
              << "     -- RX_LANE_SEL: 0x4 = " << 0x4 << "\n"
              << "     -- RX_LANE_DELAY: 0x5 = " << 0x5 << "\n"
              << "     -- RX_MANUAL_DELAY: 0x6 = " << 0x6 << "\n"
              << "     -- RX_LANE_DELAY_OUT: 0x7 = " << 0x7 << "\n"
              << "     -- RX_ERROR_COUNTER_STOP_VALUE: 0x8 = " << 0x8 << "\n"
              << "     -- RX_ERROR_COUNTER_MODE: 0x9 = " << 0x9 << "\n"
              << "     -- RX_ERROR_COUNTER_TARGET: 0xA = " << 0xA << "\n"
              << "     -- RX_ERROR_COUNTER_RESET_READ: 0xB = " << 0xB << "\n"
              << "     -- RX_TOTAL_VALID_STREAM_COUNTER: 0xC = " << 0xC << "\n"
              << "  RX_BRIDGE_ADDR: 0x3 << 14 = " << (0x3 << 14) << "\n"
              << "     -- RX_START_ADDR: 0x0 = " << 0x0 << "\n"
              << "     -- RX_DATA_COUNT: 0x1 = " << 0x1 << "\n"
              << "     -- RX_LOOPBACK: 0x2 = " << 0x2 << "\n"
              << "     -- RX_DATA_RATE: 0x3 = " << 0x3 << "\n"
              << "     -- RX_LOOP_FIFO: 0x4 = " << 0x4 << "\n"
              << "     -- RX_BRIDGE_EMPTY: 0x5 = " << 0x5 << "\n"
              << "     -- RX_CUR_COUNT: 0x6 = " << 0x6 << "\n"
              << "  TRIG_LOGIC_ADR: 0x5 << 14 = " << (0x5 << 14) << "\n"
              << "     -- TRIG_LOGIC_MASK: 0x0 = " << 0x0 << "\n"
              << "     -- TRIG_LOGIC_MODE: 0x1 = " << 0x1 << "\n"
              << "     -- TRIG_LOGIC_CONFIG: 0x2 = " << 0x2 << "\n"
              << "     -- TRIG_LOGIC_EDGE: 0x3 = " << 0x3 << "\n"
              << "     -- TRIG_LOGIC_DELAY: 0x4-0x7 = " << 0x4 << "\n"
              << "     -- TRIG_LOGIC_DEADTIME: 0x8 = " << 0x8 << "\n"
              << "     -- TRIG_LOGIC_EUDET_SIMPLE: 0x9 = " << 0x9 << "\n"
              << "     -- TRIG_LOGIC_MASTER_TRIGGER_COUNTER: 0xA = " << 0xA << "\n"
              ;
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
