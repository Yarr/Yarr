// ############################
// # Author: Timon Heim
// # Email: timon.heim at cern.ch
// # Project: Yarr
// # Description: RD53B testing
// # Date: June 2020
// ############################

#include <iostream>
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

    logger->info("Parsing command line parameters ...");
    int c;
    std::string cfgFilePath = "configs/JohnDoe.json";
    std::string ctrlFilePath = "configs/controller/specCfg.json";
    while ((c = getopt(argc, argv, "hc:r:")) != -1) {
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
    rd53b.init(&*hwCtrl, 0, 0);

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
        rd53b.fromFileJson(cfg);
        cfgFile.close();
    } else {
        logger->warn("Config file not found, using default!");
        // Write default to file
        std::ofstream newCfgFile(cfgFilePath);
        json cfg;
        rd53b.toFileJson(cfg);
        newCfgFile << std::setw(4) << cfg;
        newCfgFile.close();
    }
 
    logger->info("Enable Tx");
    hwCtrl->setCmdEnable(0);
    hwCtrl->setTrigEnable(0x0);

    logger->info("Configure chip ...");
    rd53b.configureInit();
    rd53b.configureGlobal();

    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    rd53b.setReg(7, 340, 1, 1, 1, 0); // Enable first pixel
    //rd53b.setReg(4, 6, 1, 1, 1, 0); // Enable first pixel
    //rd53b.setReg(5, 5, 1, 1, 1, 0); // Enable first pixel
    //rd53b.setReg(6, 6, 1, 1, 1, 0); // Enable first pixel
    rd53b.configurePixels();
    while(!hwCtrl->isCmdEmpty());

    logger->info("... done!");
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    hwCtrl->setRxEnable(0);
    std::this_thread::sleep_for(std::chrono::milliseconds(10));

    
    /*
    rd53b.writeRegister(&Rd53b::MonitorEnable, 1);
    rd53b.writeRegister(&Rd53b::MonitorV, 1);
    rd53b.writeRegister(&Rd53b::MonitorI, 16);
    rd53b.writeRegister(&Rd53b::DiffPreampL, 0);
    rd53b.writeRegister(&Rd53b::DiffPreampR, 0);
    rd53b.writeRegister(&Rd53b::DiffPreampTL, 0);
    rd53b.writeRegister(&Rd53b::DiffPreampTR, 0);
    rd53b.writeRegister(&Rd53b::DiffPreampM, 0);
    */
    rd53b.writeRegister(&Rd53b::InjVcalHigh, 1200);
    rd53b.writeRegister(&Rd53b::InjVcalMed, 2000);
    rd53b.writeRegister(&Rd53b::InjVcalRange, 1);
    

    /*
    logger->info("Done init, starting in 4s");
    std::this_thread::sleep_for(std::chrono::seconds(4));
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    for (unsigned int i=0; i<1023; i+=250) {
        logger->info("{}", i);
        rd53b.writeRegister(&Rd53b::DiffPreampM, i);
        //rd53b.writeRegister(&Rd53b::InjVcalHigh, i);
        //rd53b.writeRegister(&Rd53b::InjVcalMed, i);
        //rd53b.writeRegister(&Rd53b::DiffTh1M, i/4);
        while(!hwCtrl->isCmdEmpty());
        std::this_thread::sleep_for(std::chrono::seconds(5));
    }
    */

    rd53b.writeRegister(&Rd53b::Latency, 36);
    //rd53b.writeRegister(&Rd53b::InjDigEn, 1);
   
    std::array<unsigned, 32> tags = {0};

    const unsigned bins = 100;
    const unsigned scaler = 10;
    std::array<unsigned, bins> scurve = {0};

    std::array<uint32_t, 32> m_trigWord;
    unsigned m_trigMultiplier = 32;
    unsigned delay = 32;

    m_trigWord.fill(0xAAAAAAAA);
    
    // Injection command
    std::array<uint16_t, 3> calWords = rd53b.genCal(16, 0, 0, 1, 0, 0);
    m_trigWord[31] = 0xAAAA0000 | calWords[0];
    m_trigWord[30] = ((uint32_t)calWords[1]<<16) | calWords[2];
    
    uint64_t trigStream = 0;

    // Generate stream of ones for each trigger
    uint64_t one = 1;
    for (unsigned i=0; i<m_trigMultiplier; i++)
        trigStream |= (one << i);
    trigStream = trigStream << delay%8;

    for (unsigned i=0; i<(m_trigMultiplier/8)+1; i++) {
        if (((30-(delay/8)-i) > 2) && delay > 30) {
            uint32_t bc1 = (trigStream >> (2*i*4)) & 0xF;
            uint32_t bc2 = (trigStream >> ((2*i*4)+4)) & 0xF;
            m_trigWord[30-(delay/8)-i] = ((uint32_t)rd53b.genTrigger(bc1, 2*i)[0] << 16) |  rd53b.genTrigger(bc2, (2*i)+1)[0];
        } else {
            logger->error("Delay is either too small or too large!");
        }
    }

    // Rearm
    std::array<uint16_t, 3> armWords = rd53b.genCal(16, 1, 0, 0, 0, 0);
    m_trigWord[1] = 0xAAAA0000 | armWords[0];
    m_trigWord[0] = ((uint32_t)armWords[1]<<16) | armWords[2];
    
    hwCtrl->setTrigConfig(INT_COUNT);
    hwCtrl->setTrigFreq(500);
    hwCtrl->setTrigCnt(20);
    hwCtrl->setTrigWord(&m_trigWord[0], 32);
    hwCtrl->setTrigWordLength(32);

    logger->info("Trigger buffer set to:");
    for (unsigned i=0; i<32; i++) {
      logger->info("[{}: 0x{:x}", 31-i, m_trigWord[31-i]);
    }
    
    for (unsigned cal=0; cal<bins; cal++) {
        rd53b.writeRegister(&Rd53b::InjVcalHigh, 2000+(cal*scaler));
        rd53b.readRegister(&Rd53b::InjVcalHigh);
        while(!hwCtrl->isCmdEmpty());
        //rd53b.sendClear(15);
        //while(!hwCtrl->isCmdEmpty());
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        unsigned counter = 0;
            
        //hwCtrl->setTrigEnable(0x1);

        //while(!hwCtrl->isTrigDone());
        //std::this_thread::sleep_for(std::chrono::milliseconds(10));
        //hwCtrl->setTrigEnable(0x0);
        
        //logger->info("Reading data");
        RawData *data = hwCtrl->readData();
        while (data) {
            if  (data) {
                logger->info("Read {} words", data->words);
                std::pair<uint32_t, uint32_t> answer = rd53bTest::decodeSingleRegRead(data->buf[0], data->buf[1]);
                logger->info("Answer: {} {}", answer.first, answer.second);
                if (data->words>2) {
                    answer = rd53bTest::decodeSingleRegRead(data->buf[1], data->buf[2]);
                    logger->info("Answer: {} {}", answer.first, answer.second);
                }
                /*for (unsigned j=0; j<data->words; j+=2) {
                    unsigned tag = (data->buf[j] & 0x7F800000) >> 23;
                    unsigned ccol = (data->buf[j] & 0x007E0000) >> 17;
                    if (ccol > 0) {
                        counter++;
                        if(tag<32)
                            tags[tag]++;
                    }
                    logger->info("[{}] = 0x{:x} = Tag({}) CCOL({})", j, data->buf[j], tag, ccol);
                    logger->info("[{}] = 0x{:x} ", j+1, data->buf[j+1]);
                }*/
                delete data;
            }
            data = hwCtrl->readData();
        }

        scurve[cal] = counter;
        logger->info("[{}] = {}", cal*scaler, counter);
    }
    
    
    for (unsigned i=0; i<32; i++)
        logger->info("[{}] = {}", i, tags[i]);

    for (unsigned i=0; i<bins; i++)
        std::cout << scurve[i] << " ";
        //logger->info("[{}] = {}", i*scaler, scurve[i]);
    std::cout << std::endl;

    logger->info("... done! bye!");
    hwCtrl->disableRx();
    return 0;
}
