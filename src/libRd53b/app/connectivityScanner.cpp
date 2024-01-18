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

bool endswith(const std::string &str, const std::string &suffix) {
    return str.size() >= suffix.size() &&
           str.compare(str.size() - suffix.size(), suffix.size(), suffix) == 0;
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
    std::string chip_config_path = "configs/"; // path to create chip configs
    std::string chip_config_filename = chip_config_path; // path/filename for the connectivity config

    while ((c = getopt(argc, argv, "hr:c:o:")) != -1) {
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
		chip_config_filename = chip_config_path;
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

    SpecCom mySpec(specNum);
    logger->info("Scanning connectivity on Spec Card {}", specNum);
    json specStatus = mySpec.getStatus();
    //std::cout<<specStatus<<std::endl; // this works
    // TODO: need a map?

    std::string rx_speed = specStatus["rx_speed"]; // gives e.g. 1280Mbps
    std::string channel_cfg = specStatus["channel_configuration"]; // gives e.g. 16x1

    int readout_speed = std::stoi(rx_speed.substr(0, rx_speed.find('M'))); // extract from e.g. 1280Mbps string position 0 with a length up to M
    int cdrclksel = std::log2(1280/readout_speed); // get exponent of the clock divider 2^x
    int nlanes = std::stoi(channel_cfg.substr(channel_cfg.find('x')+1, 1)); // get 1 from 16x1 or 4 from 4x4
    int nrx = std::stoi(channel_cfg.substr(0, channel_cfg.find('x'))); // get 16 from 16x1 or 4 from 4x4
    int ntx = 4;

    std::string fe_type = specStatus["fe_chip_type"]; // what todo with this?
    logger->info("fe_type: {}", fe_type);
    fe_type = "rd53b";
    std::string fe_type_upper = fe_type;
    std::transform( fe_type_upper.begin(), fe_type_upper.end(), fe_type_upper.begin(), ::toupper );

    // directory and file names
    if (hw_controller_filename == "") {
	hw_controller_filename = "configs/controller/specCfg-"; // use absolute path here, "~/Yarr" doesn't work
	hw_controller_filename += fe_type + "-" + channel_cfg + ".json";
    }
    logger->info(hw_controller_filename);

    // if no ".json" in file name assume it's a directory
    if ( connectivity_filename.find(".json") == std::string::npos ) { // "find" returns the position of the first character of the first match. If no matches were found, the function returns string::npos.
	// if connectivity and chip configs are in the same directory then the chip config path in the connectivity file should be one layer higher
	if ( endswith(connectivity_filename, "/") ) {
	    connectivity_filename = connectivity_filename.substr(0, connectivity_filename.find_last_of("/"));
	}
	if ( endswith(chip_config_path, "/") ) {
	    chip_config_path = chip_config_path.substr(0, chip_config_path.find_last_of("/"));
	    chip_config_filename = chip_config_path;
	}
	if (connectivity_filename == chip_config_path) {
	    if ( connectivity_filename.find_last_of("/") != std::string::npos ) {
		chip_config_filename.substr(chip_config_filename.find_last_of("/"), std::string::npos);
	    } else {
		chip_config_filename = "";
	    }
	}

	connectivity_filename += "/" + fe_type+"_connectivity.json";
	logger->info("Did not find connectivity file extension, creating connectivity file {}.", connectivity_filename);
    }

    // if finds a connectivity path
    if ( connectivity_filename.find_last_of("/") != std::string::npos ) {
	std::string connectivity_path = connectivity_filename.substr(0, connectivity_filename.find_last_of("/"));
	if ( !std::filesystem::exists(connectivity_path) ) {
	    logger->info("Connectivity config directory {} doesn't exist, creating...", connectivity_path);
	    std::filesystem::create_directories(connectivity_path);
	}
    }

    if ( chip_config_path != "" && !std::filesystem::exists(chip_config_path) ) {
	logger->info("Chip config directory {} doesn't exist, creating...", chip_config_path);
	std::filesystem::create_directories(chip_config_path);
    }


    // jsons
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

    json jconnectivity;
    jconnectivity["chipType"] = fe_type_upper;
    jconnectivity["chips"] = json::array(); // declare an empty list


    for (int _tx = 0; _tx < ntx; _tx++) {
	// TODO
	hw->setupMode(); //?
	hw->setTrigEnable(0); //?
	hw->setCmdEnable(_tx); //?
	hw->disableRx();

	for (int _rx = 0; _rx < nrx; _rx++) {

	    Rd53b fe;
	    fe.init(&*hw, FrontEndConnectivity(_tx, _rx));

	    //auto fe = init_fe(hw, 0, 0);
	    //auto feCfg = dynamic_cast<FrontEndCfg*>(fe.get());
	    //std::string current_chip_name = cfg->getName();


	    // assuming RD53b quads (can be made more generic for triplets?) and 1.28GHz
	    json cfg;
	    cfg["RD53B"]["Parameter"]["ChipId"] = 16; // set chip ID to 16 to broadcast

	    // lane setting needed hmmmmmmmmm
	    cfg["RD53B"]["GlobalConfig"]["AuroraActiveLanes"] = (1 << nlanes)-1; //aurora active lanes = (2^nlanes)-1

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
	    std::string filename = "20UPGFC"+chip_sn.str()+".json"; // or save as chip_name.json?
	    std::ofstream newCfgFile(chip_config_path+"/"+filename);
	    newCfgFile << std::setw(4) << cfg;
	    newCfgFile.close();

	    if (chip_config_filename != "") filename = chip_config_filename + "/" + filename;

	    json jchipconnectivity;
	    jchipconnectivity["config"] = filename;
	    jchipconnectivity["path"] = "relToCon";
	    jchipconnectivity["tx"] = _tx;
	    jchipconnectivity["rx"] = _rx;
	    jchipconnectivity["enable"] = 1;
	    jchipconnectivity["locked"] = 0;

	    jconnectivity["chips"].push_back(jchipconnectivity);

	    //read firmware to determine hardware type, speed, FE chip type
	    // what if firmware is wrong? --> user
	    // scan through spec
	    // scan through polarities
	    // broadcast individual lanes
	    hw->disableRx();
	    }
	}

	std::ofstream newConnectivityFile(connectivity_filename);
	newConnectivityFile << std::setw(4) << jconnectivity;
	newConnectivityFile.close();
	return 0;
}
