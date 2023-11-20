// ############################
// # Author: Timon Heim
// # Email: timon.heim at cern.ch
// # Project: Yarr
// # Description: RD53B testing
// # Date: June 2020
// ############################

#include <iostream>
#include <sstream>
#include <string>
#include <fstream>
#include <iomanip>

#include "storage.hpp"
#include "logging.h"
#include "LoggingConfig.h"

#include "ScanHelper.h"
#include "Bookkeeper.h"

#include "Rd53b.h"

auto logger = logging::make_log("rd53bTest");

void printHelp() {
    std::cout << "-c <string> : path to config" << std::endl;
}

namespace rd53bTest {
    std::pair<uint32_t, uint32_t> decodeSingleRegRead(uint32_t higher, uint32_t lower) {
        if ((higher & 0x55000000) == 0x55000000) {
            return std::make_pair((lower>>16)&0x3FF, lower&0xFFFF);
        } else if ((higher & 0x99000000) == 0x99000000) {
            return std::make_pair((higher>>10)&0x3FF, ((lower>>26)&0x3F)+((higher&0x3FF)<<6));
        } else {
            logger->error("Could not decode reg read!");
            return std::make_pair(999, 666);
        }
        return std::make_pair(999, 666);
    }

    std::pair<uint32_t, uint32_t> singleRegRead(HwController *hwCtrl) {
        std::vector<RawDataPtr> dataVec = hwCtrl->readData();
        RawDataPtr data;
        std::pair<uint32_t, uint32_t> answer(999, 666);
        int timeout = 0;
        if  (dataVec.size() > 0) {
            data = dataVec[0];
            answer = Rd53b::decodeSingleRegRead(data->get(0), data->get(1));
        }
        return answer;
    }
}

int main (int argc, char *argv[]) {
    // Setup logger with some defaults
    std::string defaultLogPattern = "[%T:%e]%^[%=8l][%=15n]:%$ %v";
    spdlog::set_pattern(defaultLogPattern);
    json j; // empty
    j["pattern"] = defaultLogPattern;
    j["log_config"][0]["name"] = "all";
    j["log_config"][0]["level"] = "info";
    logging::setupLoggers(j);
 
    logger->info("\033[1;31m###################\033[0m");
    logger->info("\033[1;31m# RD53B Test Tool #\033[0m");
    logger->info("\033[1;31m###################\033[0m");
    logger->info("Do not use unless you know what you are doing!");
    logger->info("Do not ask questions related to this tool, as you should know what you are doing!");
    
    std::time_t now = std::time(NULL);
    struct tm *lt = std::localtime(&now);
    char c_timestamp[20];
    strftime(c_timestamp, 20, "%F_%H:%M:%S", lt);
    logger->info("Timestamp: {}", c_timestamp);
    std::string timestamp = c_timestamp;

    logger->info("Parsing command line parameters ...");
    int c;
    std::string cfgFilePath = "configs/JohnDoe.json";
    std::string ctrlFilePath = "configs/controller/specCfg.json";
    std::string outputFolder = ".";
    while ((c = getopt(argc, argv, "hc:r:o:")) != -1) {
        int count = 0;
        switch (c) {
            case 'h':
                printHelp();
                return 0;
                break;
            case 'r':
                ctrlFilePath = std::string(optarg);
                break;
            case 'c':
                cfgFilePath = std::string(optarg);
                break;
            case 'o':
                outputFolder = std::string(optarg);
                break;
            default:
                spdlog::critical("No command line parameters given!");
                return -1;
        }
    }

    logger->info("Chip config file path  : {}", cfgFilePath);
    logger->info("Ctrl config file path  : {}", ctrlFilePath);

    logger->info("\033[1;31m#################\033[0m");
    logger->info("\033[1;31m# Init Hardware #\033[0m");
    logger->info("\033[1;31m#################\033[0m");

    logger->info("-> Opening controller config: {}", ctrlFilePath);

    std::unique_ptr<HwController> hwCtrl = nullptr;
    json ctrlCfg;
    try {
        ctrlCfg = ScanHelper::openJsonFile(ctrlFilePath);
        hwCtrl = ScanHelper::loadController(ctrlCfg);
    } catch (std::runtime_error &e) {
        logger->critical("Error opening or loading controller config: {}", e.what());
        return -1;
    }
    
    hwCtrl->runMode();
    hwCtrl->setTrigEnable(0);
    hwCtrl->disableRx();

    Bookkeeper bookie(&*hwCtrl, &*hwCtrl);
    
    logger->info("\033[1;31m###################\033[0m");
    logger->info("\033[1;31m##  Chip Config  ##\033[0m");
    logger->info("\033[1;31m###################\033[0m");

    Rd53b rd53b;
    rd53b.init(&*hwCtrl, FrontEndConnectivity(0,0));

    std::ifstream cfgFile(cfgFilePath);
    if (cfgFile) {
        // Load config
        logger->info("Loading config file: {}", cfgFilePath);
        json cfg;
        try {
            cfg = ScanHelper::openJsonFile(cfgFilePath);
        } catch (std::runtime_error &e) {
            logger->error("Error opening chip config: {}", e.what());
            throw(std::runtime_error("loadChips failure"));
        }
        rd53b.loadConfig(cfg);
        cfgFile.close();
    } else {
        logger->warn("Config file not found, using default!");
        // Write default to file
        std::ofstream newCfgFile(cfgFilePath);
        json cfg;
        rd53b.writeConfig(cfg);
        newCfgFile << std::setw(4) << cfg;
        newCfgFile.close();
    }
 
    logger->info("Enable Tx");
    hwCtrl->setCmdEnable(0);
    hwCtrl->setTrigEnable(0x0);

    logger->info("Configure chip ...");
    rd53b.configureInit();
    rd53b.configureGlobal();
    rd53b.configurePixels();


    rd53b.writeRegister(&Rd53b::DiffTh1L, 500);
    rd53b.writeRegister(&Rd53b::DiffTh1R, 500);
    rd53b.writeRegister(&Rd53b::DiffTh1M, 500);
    rd53b.writeRegister(&Rd53b::DiffVff, 200);


    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    hwCtrl->setRxEnable(0);

    logger->info("Binary file: {}", (outputFolder + "/" + timestamp + "_readback.bin"));
    std::ofstream binOut((outputFolder + "/" + timestamp + "_readback.bin"), std::ios::binary);
    
    // Read counter
    rd53b.writeRegister(&Rd53b::CmdErrCnt, 0);
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    rd53b.readRegister(&Rd53b::CmdErrCnt);
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    logger->info("CmdErrCnt: {}", rd53bTest::singleRegRead(hwCtrl.get()).second);

    unsigned ok = 0;
    unsigned total = 2000;

    for (unsigned n=0; n<total; n++) {
        if (n%100==0) logger->info("Cycle {}/{}", n, total);
        
        uint32_t addr = rd53b.DiffPreampM.addr();
        uint32_t val = rd53b.DiffPreampM.read();
        rd53b.writeRegister(&Rd53b::DiffPreampM, val);
        rd53b.sendGlobalPulse(16);
        rd53b.readRegister(&Rd53b::DiffPreampM);
        while(!hwCtrl->isCmdEmpty());
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        
        std::pair<uint32_t, uint32_t> answer(0, 0), answer2(0, 0);

        std::vector<RawDataPtr> dataVec = hwCtrl->readData();
        RawDataPtr data;
        int timeout = 0;
        if  (dataVec.size() > 0) {
            data = dataVec[0];
            answer = Rd53b::decodeSingleRegRead(data->get(0), data->get(1));
            if (data->getSize()>2) {
                answer2 = Rd53b::decodeSingleRegRead(data->get(2), data->get(3));
            }
            binOut.write((char*) data->getBuf(), data->getSize()*4);
        }

        if (answer.first == addr || answer2.first == addr) {
            if (answer.second == val || answer2.second == val) {
               ok++;
            }
        }

    }
    
    hwCtrl->flushBuffer();
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    
    // Read counter
    rd53b.readRegister(&Rd53b::CmdErrCnt);
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    std::pair<uint32_t, uint32_t> readRegCmdErr = rd53bTest::singleRegRead(hwCtrl.get());
    logger->info("CmdErrCnt: {}", readRegCmdErr.second);
    
    rd53b.writeRegister(&Rd53b::CmdErrCnt, 0);
    rd53b.writeRegister(&Rd53b::SkippedTrigCnt, 0);
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
    
    rd53b.readRegister(&Rd53b::CmdErrCnt);
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    logger->info("CmdErrCnt: {} Reset ?", rd53bTest::singleRegRead(hwCtrl.get()).second);
    
    rd53b.readRegister(&Rd53b::SkippedTrigCnt);
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    logger->info("SkippedTrigCnt: {} = 0 ?", rd53bTest::singleRegRead(hwCtrl.get()).second);

#ifdef MONEATER
    std::string cmd = "echo \"success\treadRegCmdErr\n" + std::to_string(ok) +"\t" + std::to_string(readRegCmdErr.second) + "\" | python3 ~/moneater/moneater.py --host 127.0.0.1 --port 8086 --user <user> --password <password> --database betsee --table rd53b_com_test eaters.tabeater.TabEater";
    
    FILE *gnu = popen(cmd.c_str(), "w");
    pclose(gnu);

#endif

    logger->info("{} out of {} ok", ok, total);
    
    logger->info("Binary file: {}", (outputFolder + "/" + timestamp + "_trigger.bin"));
    std::ofstream trigOut((outputFolder + "/" + timestamp + "_trigger.bin"), std::ios::binary);
    
    rd53b.sendClear(16);
    while(!hwCtrl->isCmdEmpty());
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    hwCtrl->flushBuffer();
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    
    ok = 0;
    uint32_t words_received;
    total = 500;

    logger->info("Trigger test");
    for (unsigned n = 0; n<total; n++) {
        if (n%100==0) logger->info("Cycle {}/{}", n, total);
        // Send trigger
        hwCtrl->writeFifo(((uint32_t)Rd53b::genTrigger(0xF, (2*n)%50)[0] << 16) |  Rd53b::genTrigger(0xF, ((2*n)%50)+1)[0]);
        while(!hwCtrl->isCmdEmpty());
        std::this_thread::sleep_for(std::chrono::milliseconds(50));

        std::vector<RawDataPtr> dataVec = hwCtrl->readData();
        RawDataPtr data;
        if  (dataVec.size() > 0) {
            data = dataVec[0];
            logger->debug("Received {} words", data->getSize());
            for (unsigned i=0; i<data->getSize();i+=2) {
                logger->debug("[{}] = {:x} {:x}", i, data->get(i), data->get(i+1));
                uint32_t tag = (data->get(i) & 0x7F800000) >> 23;
                logger->debug("Tag: {} should be {}", tag, (((2*n%50)*4)+i/2));
                if (tag == (((2*n%50)*4)+i/2)) {
                    ok++;
                }
                words_received+=data->getSize();
            }
            trigOut.write((char*) data->getBuf(), data->getSize()*4);
        }

    }
    
    logger->info("{} out of {} ok", ok, total*8);
    logger->info("Received {} words", words_received);
    
    hwCtrl->flushBuffer();
    
    rd53b.readRegister(&Rd53b::CmdErrCnt);
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    readRegCmdErr = rd53bTest::singleRegRead(hwCtrl.get());
    logger->info("CmdErrCnt: {}", readRegCmdErr.second);
    
    rd53b.readRegister(&Rd53b::SkippedTrigCnt);
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    std::pair<uint32_t, uint32_t> skipTrig = rd53bTest::singleRegRead(hwCtrl.get());
    logger->info("SkippedTriggerCnt: {}", skipTrig.second);

#ifdef MONEATER
    std::string cmd2 = "echo \"success\ttrigCmdErr\tskippedTrigCnt\n" + std::to_string(ok) +"\t" + std::to_string(readRegCmdErr.second) + "\t" + std::to_string(skipTrig.second) + "\" | python3 ~/moneater/moneater.py --host 127.0.0.1 --port 8086 --user <user> --password <password> --database betsee --table rd53b_trig_test eaters.tabeater.TabEater";
    
    FILE *gnu2 = popen(cmd2.c_str(), "w");
    pclose(gnu2);
#endif

    logger->info("... done! bye!");
    hwCtrl->disableRx();
    binOut.close();
    trigOut.close();
    return 0;
}
