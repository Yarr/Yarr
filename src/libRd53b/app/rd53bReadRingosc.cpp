// ############################
// # Author: Maria Mironova
// # Email: maria.mironova at cern.ch
// # Project: Yarr
// # Description: Ring oscillator register reading
// # Date: December 2020
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

auto logger = logging::make_log("rd53bReadRingosc");

void printHelp() {
    std::cout << "-c <string> : path to config" << std::endl;
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
 
    logger->info("\033[1;31m##############################\033[0m");
    logger->info("\033[1;31m# RD53B Ring Oscillator Scan #\033[0m");
    logger->info("\033[1;31m##############################\033[0m");

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

    int sldoTrimD=0;

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
	sldoTrimD=cfg["RD53B"]["GlobalConfig"]["SldoTrimD"];
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


    std::this_thread::sleep_for(std::chrono::milliseconds(1));
    hwCtrl->setRxEnable(0);
    int globalPulseWidth=30;	
    int nPulse=11;
 
    // Bank A ring oscillators	
    for (unsigned i=0; i<8; i++){
	    rd53b.writeRegister(&Rd53b::RingOscAEn,255);
	    rd53b.writeRegister(&Rd53b::RingOscARoute,i);
	    rd53b.writeRegister(&Rd53b::RingOscAClear,1);
	    rd53b.writeRegister(&Rd53b::RingOscAClear,0);
	    rd53b.writeRegister(&Rd53b::GlobalPulseConf,8192);
	    rd53b.writeRegister(&Rd53b::GlobalPulseWidth,globalPulseWidth);
            logger->info("Sending Pulse");		
		for(int r=0; r<nPulse; r++) {
		    rd53b.sendGlobalPulse(15);
		    usleep(1000);
		    rd53b.readRegister(&Rd53b::RingOscAOut);
	  	    rd53b.writeRegister(&Rd53b::RingOscAClear,1);
	    	    rd53b.writeRegister(&Rd53b::RingOscAClear,0);
		}
	}
	
    // Bank B ring oscillators  
    for (unsigned i=0; i<34; i++){
	    rd53b.writeRegister(&Rd53b::RingOscBEnBl,1);
	    rd53b.writeRegister(&Rd53b::RingOscBEnBr,1);
	    rd53b.writeRegister(&Rd53b::RingOscBEnCapA,1);
	    rd53b.writeRegister(&Rd53b::RingOscBEnFf,1);
	    rd53b.writeRegister(&Rd53b::RingOscBEnLvt,1);
	    rd53b.writeRegister(&Rd53b::RingOscBRoute,i);
	    rd53b.writeRegister(&Rd53b::RingOscBClear,1);
	    rd53b.writeRegister(&Rd53b::RingOscBClear,0);
	    rd53b.writeRegister(&Rd53b::GlobalPulseConf,16384);
	    rd53b.writeRegister(&Rd53b::GlobalPulseWidth,globalPulseWidth);
            logger->info("Sending Pulse");		
		for(int r=0; r<nPulse; r++) {
		    rd53b.sendGlobalPulse(15);
		    usleep(1000);
		    rd53b.readRegister(&Rd53b::RingOscBOut);
	  	    rd53b.writeRegister(&Rd53b::RingOscBClear,1);
	    	    rd53b.writeRegister(&Rd53b::RingOscBClear,0);
		}

	}

    // Read back registers and save to file
    std::vector<RawDataPtr> dataVec = hwCtrl->readData();
    RawDataPtr data;

    unsigned int m=1;	
    double RingValuesSum = 0;
    double RingValuesSumSquared = 0;
    double frequency=0.0;
    double error=0.0;
    double min_freq=1000.0;
    double max_freq=.0;

    std::ofstream file;
    file.open("rosc_"+std::to_string(std::time(nullptr))+".txt");

    while (dataVec.size() > 0) {
        if  (dataVec.size() > 0) {
            data = dataVec[0];
            auto answer = rd53b.decodeSingleRegRead(data->get(0), data->get(1));
            frequency=(answer.second & 0xFFF)/(2*(globalPulseWidth)*0.025);
            min_freq=std::min(min_freq,frequency);
            max_freq=std::max(max_freq,frequency);
	        if (!(m%nPulse)){
			frequency=RingValuesSum/(nPulse-1);
			error=(max_freq-min_freq)/2;
		        logger->info("Mean frequency ROSC {}: {} +/- {} Mhz",m/10,frequency,error);
			file << m/10-1 << "\t" << frequency << "\t" << error << "\n";
			RingValuesSum = 0;
			RingValuesSumSquared = 0;
			min_freq=1000.0;
			max_freq=0.0;
		} else {
                        RingValuesSum+= frequency;
                        RingValuesSumSquared += pow(frequency, 2);
		}		
		m++;
        }

        dataVec = hwCtrl->readData();
    }
    file.close();

    logger->info("... done! bye!");
    hwCtrl->disableRx();
    return 0;
}
