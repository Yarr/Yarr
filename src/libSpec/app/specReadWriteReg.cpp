#include <cstdint>
#include <iomanip>
#include <string>
#include <filesystem>
namespace fs = std::filesystem;
#include <unistd.h>

#include "SpecCom.h"
#include "ScanHelper.h"
#include "SpecTxCore.h"
#include <map>

class specReg {
    public:
        specReg() {
            for (auto const& [key, val] : baseMap) {
                baseMap_r[val] = key;
            }
            for (auto const& [key, val] : regMap) {
                for (auto const& [key2, val2] : val) {
                    regMap_r[key][val2] = key2;
                }
            }
        }
        ~specReg() = default;

        void printHelp() {
            for (auto const& [key, val] : baseMap) {
                std::cout << "  " << val << ": " << key << std::endl;
                for (auto const& [key2, val2] : regMap[val]) {
                    int nchar = 30 - val2.size();
                    std::cout << "    -- " << val2 << " : " << std::string(nchar, ' ') << "0x" << std::hex << key2 << std::dec << " = " << key2 << std::endl;
                }
            }
        }

        std::map<unsigned int, std::string> baseMap = {
            {0x1 << 14, "TX_CORE"},
            {0x2 << 14, "RX_CORE"},
            {0x3 << 14, "RX_BRIDGE"},
            {0x5 << 14, "TRIGGER_LOGIC"}
        };
        std::map<std::string, unsigned int> baseMap_r;

        std::map<std::string, std::map<unsigned int, std::string>> regMap = {
            {"TX_CORE", {
                    {0x0, "TX_FIFO"},
                    {0x1, "TX_ENABLE"},
                    {0x2, "TX_EMPTY"},
                    {0x3, "TRIG_EN"},
                    {0x4, "TRIG_DONE"},
                    {0x5, "TRIG_CONF"},
                    {0x6, "TRIG_FREQ"},
                    {0x7, "TRIG_TIME"},
                    {0x9, "TRIG_COUNT"},
                    {0xA, "TRIG_WORD_LENGTH"},
                    {0xB, "TRIG_WORD"},
                    {0xC, "TRIG_POINTER"},
                    {0xD, "PULSE_WORD"},
                    {0xE, "PULSE_WORD_INTERVAL"},
                    {0xF, "TOGGLE_TRIG_ABORT"},
                    {0x10, "TX_POLARITY"},
                    {0x11, "SYNC_WORD"},
                    {0x12, "SYNC_WORD_INTERVAL"},
                    {0x13, "IDLE_WORD"},
                    {0x14, "TRIG_EXTEND_INTERVAL"},
                    {0x15, "TRIG_ENCODER_ENABLE"},
                    {0x16, "TRIG_CODE_READY_COUNT"},
                    {0x17, "BRAM_BUSY"},
                    {0x18, "BRAM_ACK_COUNT"},
                    {0x19, "BRAM_BUSY_CYCLE_COUNT"},
                    {0x1A, "BRAM_BUSY_COUNT"},
                    {0x1B, "BRAM_BUSY_ENABLE"},
                    {0x1C, "TX_TRIG_PULSE_COUNT"},
                    {0x1D, "TX_UNDERRUN"},
                    {0x1F, "TX_OVERRUN"},
                    {0x20, "TX_ALMOST_FULL"},
                    {0x21, "TX_EMPTY"},
                    {0x22, "EXT_TRIGGER_READ_COUNT"},
                    {0x23, "EXT_TRIGGER_FULL_COUNT"},
                    {0x24, "TX_CHANNEL_SEL"},
                    {0x25, "TX_SERIAL_TRIG_COUNT"},
                    {0x26, "BRAM_FULL_THRESHOLD"},
                    {0x27, "BRAM_EMPTY_THRESHOLD"}
                }},
            {"RX_CORE", {
                    {0x0, "RX_ENABLE"},
                    {0x1, "RX_LINK_STATUS"},
                    {0x2, "RX_POLARITY"},
                    {0x3, "NUM_ACTIVE_LANES"},
                    {0x4, "LANE_SELECT"},
                    {0x5, "LANE_DELAY_SET"},
                    {0x6, "SET_DELAY_AUTO_MANUAL"},
                    {0x7, "LANE_DELAY_READ"},
                    {0x8, "ERR_COUNTER_STOP_VALUE"},
                    {0x9, "ERR_COUNTER_MODE"},
                    {0xA, "ERR_COUNTER_TARGET"},
                    {0xB, "ERR_COUNTER_RESET_READ"},
                    {0xC, "TOTAL_VALID_STREAM_COUNT"}
                }},
            {"RX_BRIDGE", {
                    {0x0, "START_ADDR"},
                    {0x1, "FIFO_COUNT"},
                    {0x2, "LOOPBACK"},
                    {0x3, "DATA_RATE"},
                    {0x4, "RX_VALID_LOCAL"},
                    {0x5, "FIFO_EMPTY"},
                    {0x6, "DMA_CUR_COUNT"},
                    {0x7, "FIFO_BUSY"},
                    {0x8, "FIFO_BUSY_CYCLE_CNT"},
                    {0x9, "FIFO_BUSY_CNT"},
                    {0xA, "FIFO_BUSY_EN"},
                    {0xB, "FIFO_BUSY_SIMPLE_MODE"},
                    {0xC, "FIFO_FULL_THRESHOLD"},
                    {0xD, "FIFO_EMPTY_THRESHOLD"}
                }},
            {"TRIGGER_LOGIC", {
                    {0x0, "TRIG_MASK"},
                    {0x1, "TRIG_TAG_MODE"},
                    {0x2, "TRIG_LOGIC"},
                    {0x3, "TRIG_EDGE"},
                    {0x4, "CH0_DELAY"},
                    {0x5, "CH1_DELAY"},
                    {0x6, "CH2_DELAY"},
                    {0x7, "CH3_DELAY"},
                    {0x8, "DEADTIME"},
                    {0x9, "EUDET_SIMPLE_MODE"},
                    {0xA, "TRIG_PULSE_EXTENSION_INTERVAL"},
                    {0xB, "INT_TRIGGER_COUNT"},
                    {0xFF, "LOCAL_RESET"}
                }}
        };
        std::map<std::string, std::map<std::string, unsigned int>> regMap_r;
};

void print_help(specReg &reg) {
    std::cout << "Usage: ./bin/specReadWriteReg [-h] [-r <hw_controller_file>] [-w] [-o <register_option>]\n \n"
              << "Options:\n"
              << " -h                         Display help messages.\n"
              << " -r <hw_controller_file>    Specify hardware controller JSON path.\n"
              << " -w                         Whether to read (default) or write (-w)\n"
              << " -o <reg_addr>              Specify HW register address.\n"
              << " -b <base_addr>             Specify HW base register address (default TX_ADDR = " << TX_ADDR << ", consider TRIG_LOGIC_ADR = " << TRIG_LOGIC_ADR << ")\n"
              << " -v <value>                 Specify HW register write value (default 1).\n";
    std::cout << "Address Maps:\n";
    reg.printHelp();
    return;
}

int main(int argc, char **argv) {
    int c;
    int specNum = 0;
    uint32_t regValue = 0;
    
    uint32_t regOption = 0;
    uint32_t baseAddr = 0;
    std::string regOption_s = "", baseAddr_s = "";

    std::string hw_controller_filename = "";
    bool read=true;
    
    specReg reg;

    while ((c = getopt(argc, argv, "hwr:o:v:b:")) != -1) {
        switch (c) {
            case 'h':
                print_help(reg);
                return 0;
            case 'r':
                hw_controller_filename = optarg;
                break;
            case 'w':
                read=false;
                break;
            case 'o':
                regOption = std::atoi(optarg);
                regOption_s = std::string(optarg);
                break;
            case 'v':
                regValue = std::atoi(optarg);
                break;
            case 'b':
                baseAddr = std::atoi(optarg);
                baseAddr_s = std::string(optarg);
                break; 
            default:
                print_help(reg);
                return -1;
        }
    }

    // std::cout << baseAddr_s << ": " << baseAddr << ", " << regOption_s << ": " << regOption << std::endl;
    // for (auto const& [key, val] : reg.baseMap_r) {
    //     std::cout << key << " : " << val << std::endl;
    // }
    if (reg.baseMap_r.find(baseAddr_s) != reg.baseMap_r.end()) {
        // std::cout << "inferring register base from " << baseAddr_s << std::endl;
        baseAddr = reg.baseMap_r[baseAddr_s];
    }
    else {
        // std::cout << "using integer reg base"  << baseAddr << std::endl;
        baseAddr_s = reg.baseMap[baseAddr];
    }

    if (reg.regMap_r[baseAddr_s].find(regOption_s) != reg.regMap_r[baseAddr_s].end()) {
        // std::cout << "inferring register value from " << regOption_s << std::endl;
        regOption = reg.regMap_r[baseAddr_s][regOption_s];
    }
    else {
        // std::cout << "using integer reg value " << regOption << std::endl;
        regOption_s = reg.regMap[baseAddr_s][regOption];
    }

    fs::path hw_controller_path{hw_controller_filename};
    if(!fs::exists(hw_controller_path)) {
        std::cerr << "ERROR: Provided hw controller file (=" << hw_controller_filename << ") does not exist" << std::endl;
        return 1;
    }


    json jcontroller;
    jcontroller = ScanHelper::openJsonFile(hw_controller_filename);
    specNum=jcontroller["ctrlCfg"]["cfg"]["specNum"];

    SpecCom mySpec(specNum, false);

    if(read) {
        uint32_t rValue = mySpec.readSingle(baseAddr | regOption);
        std::cout << baseAddr_s << " / " << regOption_s << ": value : " << rValue << std::endl;
    }
    else {
        mySpec.writeSingle(baseAddr | regOption, regValue);
        std::cout << baseAddr_s << " / " << regOption_s << ": written with value : " << regValue << std::endl;
    }
    return 0;
}
