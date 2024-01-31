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


auto logger = logging::make_log("Rd53bConnectivityScan");

void printHelp() {
       std::cout << "Usage: ./bin/connectivityScanner [-h] [-r <hw_controller_file>] [-c <connectivity_file>] [-o <output_path>]\n\n"
              << "Options:\n"
              << "  -h                        Display this help message.\n"
              << "  -r <hw_controller_file>   Specify hardware controller JSON path (required).\n"
              << "  -c <connectivity_file>    Specify connectivity config JSON path. Default is \"configs/connectivity/auto_rd53b_setup.json\"\n"
              << "  -o <config_path>          Specify directory path for chip configs. Default is \"configs/\"\n" ;
              //<< "  -p <option>               Path relation, e.g. choose from 'relToExec' (default), 'relToCon' or 'abs' .\n"; // TODO?
}

bool endswith(const std::string &str, const std::string &suffix) {
    return str.size() >= suffix.size() &&
           str.compare(str.size() - suffix.size(), suffix.size(), suffix) == 0;
}

int main(int argc, char **argv) {
    // Setup logger with some defaults
    std::string defaultLogPattern = "[%T:%e]%^[%=8l][%=15n]:%$ %v";
    spdlog::set_pattern(defaultLogPattern);
    json j; // empty
    j["pattern"] = defaultLogPattern;
    j["log_config"][0]["name"] = "all";
    j["log_config"][0]["level"] = "info";
    // switch this logger off due to always using default global config
    j["log_config"][1]["name"] = "Rd53bGlobalCfg";
    j["log_config"][1]["level"] = "critical";
    j["log_config"][2]["name"] = "Rd53bPixelCfg";
    j["log_config"][2]["level"] = "critical";
    j["log_config"][3]["name"] = "Rd53bConnectivityScan";
    j["log_config"][3]["level"] = "debug";
    logging::setupLoggers(j);

    // args
    int c;
    std::string hw_controller_filename = "";
    std::string connectivity_filename = "configs/connectivity/auto_rd53b_setup.json";
    std::string chip_config_path = "configs/"; // path to create chip configs
    std::string chip_config_filename = chip_config_path; // path/filename for the connectivity config
    std::string path_relation = "relToExec"; // path relation
    int sleep = 1000;

    while ((c = getopt(argc, argv, "hr:c:o:p:t:")) != -1) {
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
		chip_config_path = optarg; // used to store the configs
		chip_config_filename = chip_config_path; // used as link in connectivity config
		break;
	    case 'p':
		path_relation = optarg; // TODO
		break;
	    case 't':
		sleep = atoi(optarg); // TODO
		break;
	    default:
		logger->critical("Invalid command line parameter(s) given!");
		return -1;
	}
    }

    // check controller config
    if(hw_controller_filename.empty()) {
	logger->critical("Controller config required (-r)");
	std::cout << "Rerun with -h for more information\n";
	return -1;
    }
    fs::path hw_controller_path{hw_controller_filename};
    if(!fs::exists(hw_controller_path)) {
        std::cerr << "ERROR: Provided hw controller file (=" << hw_controller_filename << ") does not exist" << std::endl;
        return 1;
    }

    logger->debug("{} {} {}", hw_controller_filename, connectivity_filename, chip_config_path);

    // TODO: generic hardware controller
    // Init spec
    logger->info("Init spec");
    int specNum = 0;

    SpecCom mySpec(specNum);
    logger->info("Scanning connectivity on Spec Card {}", specNum);
    json specStatus = mySpec.getStatus();
    //std::cout<<specStatus<<std::endl; // this works

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

    //// directory and file names
    if ( !std::filesystem::exists(chip_config_path) ) {
	logger->info("Chip config directory \"{}\" doesn't exist, creating...", chip_config_path);
	std::filesystem::create_directories(chip_config_path);
    }

    if ( endswith(chip_config_path, "/") ) {
	chip_config_path = chip_config_path.substr(0, chip_config_path.find_last_of("/"));
	chip_config_filename = chip_config_path;
    }

    // if no ".json" in file name assume it's a directory
    if ( connectivity_filename.find(".json") == std::string::npos ) { // "find" returns the position of the first character of the first match. If no matches were found, the function returns string::npos.
	// strip last "/" if existing
	if ( endswith(connectivity_filename, "/") ) {
	    connectivity_filename = connectivity_filename.substr(0, connectivity_filename.find_last_of("/"));
	}

	// if connectivity and chip configs are in the same directory (e.g. -c test -o test) then the chip config path in the connectivity file should be one layer higher
	//if (connectivity_filename == chip_config_path) {
	    //if ( connectivity_filename.find_last_of("/") != std::string::npos ) {
		//chip_config_filename.substr(chip_config_filename.find_last_of("/"), std::string::npos);
	    //} else {
		//chip_config_filename = "";
	    //}
	//}

	connectivity_filename += "/" + fe_type+"_connectivity.json";
	logger->info("Creating connectivity file '{}'", connectivity_filename);
    }

    logger->debug("chip_config_path: {}", chip_config_path);
    logger->debug("chip_config_filename: {}", chip_config_filename);

    // if finds a connectivity path
    if ( connectivity_filename.find_last_of("/") != std::string::npos ) {
	std::string connectivity_path = connectivity_filename.substr(0, connectivity_filename.find_last_of("/"));
	if ( !std::filesystem::exists(connectivity_path) ) {
	    logger->info("Connectivity config directory \"{}\" doesn't exist, creating...", connectivity_path);
	    std::filesystem::create_directories(connectivity_path);
	}
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

	    // assuming RD53b quads (can be made more generic for triplets?) and 1.28GHz
	    json cfg;
	    cfg["RD53B"]["Parameter"]["ChipId"] = 16; // set chip ID to 16 to broadcast

	    // lane setting needed hmmmmmmmmm
	    cfg["RD53B"]["GlobalConfig"]["AuroraActiveLanes"] = (1 << nlanes)-1; //aurora active lanes = (2^nlanes)-1

	    fe.loadConfig(cfg);

	    // configure chip with the correct readout speed and other registers
	    logger->info("Poking Tx {}, Rx{}......", _tx, _rx);
	    hw->setRxEnable(_rx);
	    hw->checkRxSync();

	    // if configured only once first chip often fails (e.g. no chip ID read) also with 1s sleep
	    // configuring twice works
	    fe.configureInit();
	    fe.configureGlobal();
	    std::this_thread::sleep_for(std::chrono::microseconds(sleep));
	    fe.configureGlobal();
	    std::this_thread::sleep_for(std::chrono::microseconds(sleep));

	    uint8_t chipId = fe.getChipId();
	    logger->debug("Get 2-LSB chip ID: {}", chipId);
	    if(chipId == 255) continue;

	    // try establish com assuming quad chip ID
	    chipId += 12; // assuming quad to be the majority
	    logger->info("Configure chip again with chipId = {}", chipId); // have to do this again in order to be able to read out efuses
	    cfg["RD53B"]["Parameter"]["ChipId"] = chipId;
	    fe.loadConfig(cfg);
	    fe.configureGlobal();
	    std::this_thread::sleep_for(std::chrono::microseconds(sleep));

	    // https://gitlab.cern.ch/YARR/YARR/-/issues/166
	    uint32_t efuse = fe.getEfuses(); // TODO try/except

	    logger->debug("efuse {}", efuse);

	    // if cannot read out efuse, try triplet chip ID
	    if ( !efuse ) {
		chipId -= 12; // triplet chip ID
		logger->warn("Can't read efuse, try triplet chip ID {}", chipId);
		cfg["RD53B"]["Parameter"]["ChipId"] = chipId;
		fe.loadConfig(cfg);
		fe.configureGlobal();
		std::this_thread::sleep_for(std::chrono::microseconds(sleep));
		efuse = fe.getEfuses();
		logger->debug("efuse again {}", efuse);
	    }

	    //-------------
	    std::stringstream chip_name;
	    std::stringstream chip_sn;
	    chip_name << "0x" << std::hex << efuse;
	    chip_sn << std::setw(7) << std::setfill('0') << efuse;
	    logger->debug("getEfuses: {}", chip_name.str());
	    logger->debug("chip SN {}", "20UPGFC"+chip_sn.str());
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
	    jchipconnectivity["path"] = path_relation; // TODO default is "relToExec"
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
