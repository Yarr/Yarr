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

void printHelp() {
       std::cout << "Usage: ./bin/connectivityScanner [-h] [-r <hw_controller_file>] [-c <connectivity_file>] [-o <output_path>]\n\n"
              << "Options:\n"
              << "  -h                        Display this help message.\n"
              << "  -r <hw_controller_file>   Specify hardware controller JSON path.\n"
              << "  -c <connectivity_file>    Specify connectivity config JSON path.\n"
              << "  -o <config_path>          Output chip config JSON path.\n" ;
}

//std::shared_ptr<FrontEnd> init_fe(std::unique_ptr<HwController>& hw, int tx, int rx) {

    ////std::string chip_type = jconn["chipType"];
    //auto fe = StdDict::getFrontEnd("RD53B");
    //auto cfg = dynamic_cast<FrontEndCfg*>(fe.get());
    
    //// insert tx and rx loop here
    //fe->init(&*hw, cfg);
    //// use auto conf?
    //cfg->loadConfig("");
    //return fe;
//}

int main(int argc, char **argv) {
    // Setup logger with some defaults
    std::string defaultLogPattern = "[%T:%e]%^[%=8l][%=15n]:%$ %v";
    spdlog::set_pattern(defaultLogPattern);
    json j; // empty
    j["pattern"] = defaultLogPattern;
    j["log_config"][0]["name"] = "all";
    j["log_config"][0]["level"] = "info";
    logging::setupLoggers(j);

    // home path
    std::string home;
    if(getenv("HOME")) {
	home = getenv("HOME");
    } else {
	logger->error("HOME not set, using local directory for configuration");
	home = ".";
    }

    // args
    int c;
    std::string hw_controller_filename = "";
    std::string connectivity_filename = "";
    std::string chip_config_path = "configs/";

    while ((c = getopt(argc, argv, "hr:c:o")) != -1) {
        switch (c) {
	    case 'h':
		printHelp();
		return 0;
	    case 'r':
		hw_controller_filename = optarg;
		break;
	    case 'c' :
		connectivity_filename = optarg;
		break;
	    case 'o' :
		 chip_config_path = optarg;
		break;
	    default:
		logger->critical("Invalid command line parameter(s) given!");
		return -1;
	}
    }

    // TODO: also scann through different spec IDs?
    // Init spec
    logger->info("Init spec");
    int specNum = 0;
    // temporary
    hw_controller_filename = "/home/captain/Yarr/configs/controller/specCfg-"; // use absolute path here, "~/Yarr" doesn't work

    SpecCom mySpec(specNum);
    json specStatus = mySpec.getStatus();

    std::string rx_speed = specStatus["rx_speed"]; // gives e.g. 1280Mbps
    std::string channel_cfg = specStatus["channel_configuration"]; // gives e.g. 16x1

    int readout_speed = std::stoi(rx_speed.substr(0, rx_speed.find('M'))); // extract from e.g. 1280Mbps string position 0 with a length up to M
    int cdrclksel = std::log2(1280/readout_speed); // get exponent of the clock divider 2^x
    int activelanes = std::stoi(channel_cfg.substr(channel_cfg.find('x')+1, 1)); // get 1 from 16x1 or 4 from 4x4
    int nrx = std::stoi(channel_cfg.substr(0, channel_cfg.find('x'))); // get 16 from 16x1 or 4 from 4x4
    int ntx = 4;

    // TODO: need a map?
    std::string fe_type = specStatus["fe_chip_type"];
    //std::cout<<specStatus<<std::endl; // this works
    logger->info("fe_type: {}", fe_type);
    fe_type = "rd53b";
    hw_controller_filename += fe_type + "-" + channel_cfg + ".json";
    std::cout<<hw_controller_filename<<std::endl;

    json jcontroller;
    jcontroller = ScanHelper::openJsonFile(hw_controller_filename);
    // instantiate the hw controller
    std::unique_ptr<HwController> hw;
    try {
        jcontroller = ScanHelper::openJsonFile(hw_controller_filename);
        hw = ScanHelper::loadController(jcontroller);
    } catch (std::exception& e) {
        std::cerr << "ERROR: Unable to load controller from provided config, exception caught: " << e.what() << std::endl;
        return 1;
    }

    for (int _tx = 0; _tx < ntx; _tx++) {
	// TODO
	hw->setupMode(); //?
	hw->setTrigEnable(0); //?
	hw->setCmdEnable(_tx); //?

	for (int _rx = 0; _rx < nrx; _rx++) {

	    //----
	    Rd53b fe;
	    fe.init(&*hw, FrontEndConnectivity(0, _rx));

	    //auto fe = init_fe(hw, 0, 0);
	    //auto feCfg = dynamic_cast<FrontEndCfg*>(fe.get());
	    //std::string current_chip_name = cfg->getName();

	    json cfg;
	    cfg["RD53B"]["Parameter"]["ChipId"] = 16; // set chip ID to 16 to broadcast
	    cfg["RD53B"]["GlobalConfig"]["CdrClkSel"] = cdrclksel; // set clock divider
	    cfg["RD53B"]["GlobalConfig"]["AuroraActiveLanes"] = activelanes; //aurora active lanes
	    cfg["RD53B"]["GlobalConfig"]["SerEnLane"] = 15; // depends on the chip ID TODO

	    cfg["RD53B"]["GlobalConfig"]["CmlBias0"] = 800; // update default?
	    cfg["RD53B"]["GlobalConfig"]["CmlBias1"] = 400; // update default?
	    cfg["RD53B"]["GlobalConfig"]["CmlBias2"] = 0; // update default?

	    cfg["RD53B"]["GlobalConfig"]["SerEnTap"] = 1; // update default?
	    cfg["RD53B"]["GlobalConfig"]["SerInvTap"] = 1; //  update default?

	    // not needed for this
	    //cfg["RD53B"]["GlobalConfig"]["MonitorEnable"] = 1; // update default?
	    //cfg["RD53B"]["GlobalConfig"]["MonitorI"] = 63; // is default
	    //cfg["RD53B"]["GlobalConfig"]["MonitorV"] = 32; // update default?
	    //cfg["RD53B"]["GlobalConfig"]["ServiceBlockEn"] = 1; // update default?
	    //cfg["RD53B"]["GlobalConfig"]["ServiceBlockPeriod"] = 50; // update default?

	    fe.loadConfig(cfg);

	    // configure chip with the correct readout speed and other registers
	    logger->info("Configure chip ...");
	    fe.configureInit();
	    fe.configureGlobal();
	    std::this_thread::sleep_for(std::chrono::microseconds(10));

	    logger->info("Enable Rx{}", _rx);
	    hw->setRxEnable(_rx);
	    hw->checkRxSync();

	    uint8_t chipId = fe.getChipId();
	    logger->info("Get 2-LSB chip ID: {}", chipId);
	    if(chipId == 255) continue;

	    if(channel_cfg == "16x1") {
		chipId += 12; // only for 16x1 FW, assuming quad
		cfg["RD53B"]["Parameter"]["ChipId"] = chipId;
		fe.loadConfig(cfg);

		logger->info("Configure chip again..."); // have to do this again in order to be able to read out efuses
		fe.configureInit();
		fe.configureGlobal();
		std::this_thread::sleep_for(std::chrono::microseconds(10));
	    }

	    // https://gitlab.cern.ch/YARR/YARR/-/issues/166
	    uint32_t efuse = fe.getEfuses();
	    std::stringstream chip_name;
	    std::stringstream chip_sn;
	    chip_name << "0x" << std::hex << efuse;
	    chip_sn << std::setw(7) << std::setfill('0') << efuse;
	    //logger->info("getEfuses: {}", chip_name.str());
	    //logger->info("20UPGFC"+chip_sn.str());
	    cfg["RD53B"]["Parameter"]["Name"] = chip_name.str();

	    ////// Write default to file
	    //fe.writeConfig(cfg); // fills in all the missing values with default
	    std::ofstream newCfgFile("configs/20UPGFC"+chip_sn.str()+".json"); // or save as chip_name.json?
	    newCfgFile << std::setw(4) << cfg;
	    newCfgFile.close();


	    //read firmware to determine hardware type, speed, FE chip type
	    // what if firmware is wrong? --> user
	    // scan through spec
	    // scan through polarities
	    // broadcast individual lanes
	    hw->disableRx();
	    }
	}
	return 0;
}
