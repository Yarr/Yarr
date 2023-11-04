// speccontroller: what is setup mode and what is run mode? pulse word different


// #################################
// # Author: Lingxin Meng
// # Email: lmeng at cern.ch
// # Project: Yarr
// # Description: scans...
// # Date: Nov. 2023
// ################################

#include <cstdint>
#include <string>
#include <iomanip>

#include <filesystem>
namespace fs = std::filesystem;

#include "SpecCom.h"
#include "logging.h"
#include "LoggingConfig.h"
#include "storage.hpp"
// YARR
#include "HwController.h"
#include "FrontEnd.h"
#include "AllChips.h"
#include "ScanHelper.h"
#include "Utils.h"

#include "Rd53b.h"


auto logger = logging::make_log("connectivityScanner");

//void printHelp() {
       //std::cout << "Usage: ./bin/connectivityScanner [-h] [-r <hw_controller_file>] [-c <connectivity_file>] [-t <test_size>] [-s]\n\n"
              //<< "Options:\n"
              //<< "  -h                   Display this help message.\n"
              //<< "  -r <hw_controller_file>   Specify hardware controller JSON path.\n"
              //<< "  -c <connectivity_file>    Specify connectivity config JSON path.\n"
              //<< "  -t <test_size>            Specify the error counter test size.\n"
              //<< "  -o                   Output connectivity file name\n" ;
//}

std::shared_ptr<FrontEnd> init_fe(std::unique_ptr<HwController>& hw, int tx, int rx) {

    //std::string chip_type = jconn["chipType"];
    auto fe = StdDict::getFrontEnd("RD53B");
    auto cfg = dynamic_cast<FrontEndCfg*>(fe.get());
    
    // insert tx and rx loop here
    fe->init(&*hw, tx, rx);
    // use auto conf?
    cfg->loadConfig("");
    return fe;
}

int main(int argc, char **argv) {
    // Setup logger with some defaults
    std::string defaultLogPattern = "[%T:%e]%^[%=8l][%=15n]:%$ %v";
    spdlog::set_pattern(defaultLogPattern);
    json j; // empty
    j["pattern"] = defaultLogPattern;
    j["log_config"][0]["name"] = "all";
    j["log_config"][0]["level"] = "info";
    logging::setupLoggers(j);
  	// TODO: also scann through different spec IDs
    // Init spec
    logger->info("Init spec");
	// create spec controller
    int c;	
    int specNum = 0;
    int n_lanes= 16;
    std::string hw_controller_filename = "/home/lmeng/Yarr/configs/controller/specCfg-"; // use absolute path here, "~/Yarr" doesn't work

	SpecCom mySpec(specNum);
	json specStatus = mySpec.getStatus();

	std::string rx_speed = specStatus["rx_speed"];
	std::string channel_cfg = specStatus["channel_configuration"];

	// TODO: need a map?
	//std::string fe_type = specStatus["fe_chip_type"];
	std::string fe_type = "rd53b";
	hw_controller_filename += fe_type + "-" + channel_cfg + ".json";
	std::cout<<hw_controller_filename<<std::endl;
	
	//logger->info(typeid(specStatus).name()); //????
	//std::cout<<typeid(specStatus).name()<<std::endl;
	
	//std::string s = specStatus.dump(); //doesn't work???
	//logger->info(s);
	//std::cout<<specStatus<<std::endl;
    //logger->info(specStatus);
	
	json jcontroller;
    jcontroller = ScanHelper::openJsonFile(hw_controller_filename);
	std::cout<<hw_controller_filename<<std::endl;
    // instantiate the hw controller
    std::unique_ptr<HwController> hw;
    try {
        jcontroller = ScanHelper::openJsonFile(hw_controller_filename);
        hw = ScanHelper::loadController(jcontroller);
    } catch (std::exception& e) {
        std::cerr << "ERROR: Unable to load controller from provided config, exception caught: " << e.what() << std::endl;
        return 1;
    }
    hw->setupMode(); //?
    hw->setTrigEnable(0);
    
    
    for (int _rx = 0; _rx < 1; _rx++) {
        
        //----
        Rd53b fe;
        fe.init(&*hw, 0, _rx);
        
        //auto fe = init_fe(hw, 0, 0);
        //auto feCfg = dynamic_cast<FrontEndCfg*>(fe.get());
        //std::string current_chip_name = cfg->getName();
        
        json cfg;
        cfg["RD53B"]["Parameter"]["ChipId"] = 16;
        cfg["RD53B"]["GlobalConfig"]["CdrClkSel"] = 0;
        fe.loadConfig(cfg);
        fe.writeConfig(cfg);


     
        logger->info("Enable Tx");
        hw->setCmdEnable(0);
        hw->setRxEnable(_rx);
        
        std::this_thread::sleep_for(std::chrono::seconds(1));
        
        //logger->info("Configure chip ...");
        //fe.configureInit();
        //fe.configureGlobal();

        uint8_t chipId = fe.getChipId();
        logger->info("get chip ID: {}", chipId);

        // Write default to file
        cfg["RD53B"]["Parameter"]["ChipId"] = chipId;
        cfg["RD53B"]["GlobalConfig"]["CdrClkSel"] = 0;
        fe.loadConfig(cfg);
        fe.writeConfig(cfg);
        
        // https://gitlab.cern.ch/YARR/YARR/-/issues/166
        std::stringstream chip_sn;
        chip_sn << "0x" << std::hex << fe.getEfuses();
        logger->info("getEfuses: {}", chip_sn.str());


        std::ofstream newCfgFile("/tmp/test.json");
        cfg["RD53B"]["Parameter"]["Name"] = chip_sn.str();
        newCfgFile << std::setw(4) << cfg;
        newCfgFile.close();
        //logger->info("Reading efuse of chip: {}", feCfg->getName()); 
        //---
        
        //specNum=jcontroller["ctrlCfg"]["cfg"]["specNum"];
	    

	    //read firmware to determine hardware type, speed, FE chip type
	    // what if firmware is wrong? --> user
	    // scan through spec
	    // scan through polarities
	    // broadcast individual lanes
	}
	return 0;
}
