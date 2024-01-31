// #################################
// # Author: Lingxin Meng
// # Email: lmeng at cern.ch
// # Project: Yarr
// # Description: scans through all tx and rx, broadcasts config, reads back chip ID + efuse and writes connectivity and chip configs
// # Date: Nov. 2023
// #################################

#include <cstdint>
#include <string>
#include <iomanip>
#include <list>
#include <algorithm>
#include <getopt.h>
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
	  << "  -h/--help                         Display this help message.\n"
	  << "  -r         <hw_controller_file>   Specify hardware controller JSON path (required).\n"
	  << "  -c         <connectivity_file>    Specify connectivity config directory or JSON path. Default is \"configs/connectivity/auto_rd53b_setup.json\"\n"
	  << "  -o         <config_path>          Specify directory path for chip configs. Default is \"configs/\"\n"
	  << "  -p         <string>               Path relation, e.g. choose from 'relToExec' (default), 'relToCon' or 'abs' .\n" // TODO?
	  << "  -s         <integer>              Specify the sleep time in microseconds after configuration. Default is 1000us.\n"
	  << "  --tx       <integer>              Specify the number of tx (command). Default depends on the controller.\n"
	  << "  --rx       <integer>              Specify the number of rx (data). Default depends on the controller.\n"
	  << "  --nlanes   <integer>              Specify the number of lanes per chip. Default is 1.\n";
}

bool endswith(const std::string &str, const std::string &suffix) {
    return str.size() >= suffix.size() &&
           str.compare(str.size() - suffix.size(), suffix.size(), suffix) == 0;
}

// works on list or vector etc
auto container_printer =  [](auto const & l) {
        std::string r;
        for(auto const& i: l) r += i+", ";
        r.pop_back();
        r.pop_back();
        return r;
    };

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
    //j["log_config"][3]["name"] = "Rd53bConnectivityScan";
    //j["log_config"][3]["level"] = "debug";
    logging::setupLoggers(j);

    // args
    int c;
    std::string hw_controller_filename = "";
    std::string connectivity_path = ".";
    std::string connectivity_filename = "configs/connectivity/auto_rd53b_setup.json";
    std::string chip_config_path = "configs/"; // path to create chip configs
    std::string chip_config_filename = chip_config_path; // path/filename for the connectivity config
    std::string path_relation = ""; // path relation
    int sleep = 1000;
    // TODO: override default values if entered on commandline // bool override = false;
    int nlanes = -1; // e.g. 1 from 16x1 or 4 from 4x4
    int nrx = -1; // e.g. 16 from 16x1 or 4 from 4x4
    int ntx = -1;

    const char* const short_opts = "hr:c:o:p:s:d:u:n:"; // have to include the short forms of all long options
    // https://www.ibm.com/docs/en/zos/3.1.0?topic=functions-getopt-long-command-long-option-parsing
    const option long_opts[] = {
            {"help", no_argument, nullptr, 'h'},
	    {"tx", required_argument, nullptr, 'd'}, //Downlink
            {"rx", required_argument, nullptr, 'u'}, //Uplink
            {"nlanes", required_argument, nullptr, 'n'},
            {nullptr, no_argument, nullptr, 0}
	};

    //while ((c = getopt(argc, argv, )) != -1) {
    while ((c = getopt_long(argc, argv, short_opts, long_opts, nullptr)) != -1) {
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
	    case 's':
		sleep = atoi(optarg); // TODO
		break;
	    case 'd':
		ntx = atoi(optarg);
		break;
	    case 'u':
		nrx = atoi(optarg);
		break;
	    case 'n':
		nlanes = atoi(optarg);
		break;
	    default:
		logger->critical("Invalid command line parameter(s) given!");
		return -1;
	}
    }

    // check validity of inputs
    // controller config
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

    // path relation
    std::list<std::string> paths {"relToExec", "relToCon", "abs"};
    logger->debug("input path_relation: {}", path_relation);
    if ( path_relation != "" && std::find(paths.begin(), paths.end(), path_relation) == paths.end() ) {
	logger->critical("Invalid path relation given {}! Must be from {}", path_relation, container_printer(paths));
	return -1;
    }
    if ( path_relation == "" ) path_relation = "relToExec";

    logger->debug("{} {} {}", hw_controller_filename, connectivity_filename, chip_config_path);

    logger->debug("chips");
    //ScanHelper::listChips(); // DEBUG
    logger->debug("controllers");
    //ScanHelper::listControllers(); // DEBUG

    // instantiate the hw controller
    std::unique_ptr<HwController> hw;
    json jcontroller;
    try {
        jcontroller = ScanHelper::openJsonFile(hw_controller_filename);
        hw = ScanHelper::loadController(jcontroller);
    } catch (std::exception& e) {
        std::cerr << "ERROR: Unable to load controller from provided config, exception caught: " << e.what() << std::endl;
        return 1;
    }

    //// controller json
    //json hwStatus = hw->getStatus(); // DEBUG
    //std::cout<<hwStatus<<std::endl; // DEBUG; std::out works, using logger doesn't work.

    // set number of lanes, number of rx and number of tx depending on the controller type // TODO
    //std::cout<<jcontroller["ctrlCfg"]["type"]<<std::endl; // DEBUG

    // check if values are their originally initiated values
    if (jcontroller["ctrlCfg"]["type"] == "spec") {
	if (nlanes == -1) nlanes = 1;
	if (nrx == -1) nrx = 16;
	if (ntx == -1) ntx = 4;
    } else if (jcontroller["ctrlCfg"]["type"] == "bdaq") {
	// TODO
    } else if (jcontroller["ctrlCfg"]["type"] == "emu") {
	// TODO
    } else if (jcontroller["ctrlCfg"]["type"] == "emu_Rd53a") {
	// TODO
    } else if (jcontroller["ctrlCfg"]["type"] == "emu_Star") {
	// TODO
    } else if (jcontroller["ctrlCfg"]["type"] == "FelixClient") {
	// TODO
    } else if (jcontroller["ctrlCfg"]["type"] == "Netio") {
	// TODO
    } else if (jcontroller["ctrlCfg"]["type"] == "Itsdaq") {
	// TODO
    } else if (jcontroller["ctrlCfg"]["type"] == "ku040") {
	// TODO
    } else if (jcontroller["ctrlCfg"]["type"] == "rce") {
	// TODO
    }

    logger->info("Will scan through {}(Tx) * {}(Rx) = {} channels at 1.28GHz!", ntx, nrx, ntx*nrx);

    //std::string fe_type = hwStatus["fe_chip_type"]; // only works for spec
    //logger->info("fe_type: {}", fe_type);
    std::string fe_type = "rd53b";
    std::string fe_type_upper = fe_type;
    std::transform( fe_type_upper.begin(), fe_type_upper.end(), fe_type_upper.begin(), ::toupper );

    //// directory and file names
    // connectivity
    // if no ".json" in file name assume it's a directory
    if ( connectivity_filename.find(".json") == std::string::npos ) { // "find" returns the position of the first character of the first match. If no matches were found, the function returns string::npos.
	// strip last "/" if existing
	if ( endswith(connectivity_filename, "/") ) {
	    connectivity_filename = connectivity_filename.substr(0, connectivity_filename.find_last_of("/"));
	}
	connectivity_filename += "/" + fe_type+"_connectivity.json";
	logger->info("Creating connectivity file '{}'", connectivity_filename);
    }

    // if finds a connectivity path (with or without .json)
    if ( connectivity_filename.find_last_of("/") != std::string::npos ) {
	connectivity_path = connectivity_filename.substr(0, connectivity_filename.find_last_of("/"));
	if ( !std::filesystem::exists(connectivity_path) ) {
	    logger->info("Connectivity config directory \"{}\" doesn't exist, creating...", connectivity_path);
	    std::filesystem::create_directories(connectivity_path);
	}
    }

    logger->debug("connectivity_path {}", connectivity_path);
    logger->debug("connectivity_filename {}", connectivity_filename);

    // chip configs
    // needed?
    if ( endswith(chip_config_path, "/") ) {
	chip_config_path = chip_config_path.substr(0, chip_config_path.find_last_of("/"));
	chip_config_filename = chip_config_path;
    }

    if (path_relation == "relToCon") {
	chip_config_path = connectivity_path + "/" + chip_config_path;
    }

    if ( !std::filesystem::exists(chip_config_path) ) {
	logger->info("Chip config directory \"{}\" doesn't exist, creating...", chip_config_path);
	std::filesystem::create_directories(chip_config_path);
    }

    logger->debug("chip_config_path: {}", chip_config_path);
    logger->debug("chip_config_filename: {}", chip_config_filename);

    // connectivity json
    json jconnectivity;
    jconnectivity["chipType"] = fe_type_upper;
    jconnectivity["chips"] = json::array(); // declare an empty list

    for (int _tx = 0; _tx < ntx; _tx++) {
	// TODO: check if all correct
	hw->setupMode(); // ?
	hw->setTrigEnable(0); // ?
	hw->setCmdEnable(_tx); // ?
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

	    std::stringstream chip_name;
	    std::stringstream chip_sn;
	    chip_name << "0x" << std::hex << efuse;
	    chip_sn << std::setw(7) << std::setfill('0') << efuse;
	    logger->debug("getEfuses: {}", chip_name.str());
	    logger->debug("chip SN {}", "20UPGFC"+chip_sn.str());
	    cfg["RD53B"]["Parameter"]["Name"] = chip_name.str();

	    ////// Write default to file
	    //fe.writeConfig(cfg); // fills in all the missing values with default
	    //std::string filename = "20UPGFC"+chip_sn.str()+".json"; // or save as chip_name.json?
	    std::string filename = chip_name.str()+".json"; // or save as serialNumber.json?
	    std::ofstream newCfgFile(chip_config_path+"/"+filename);
	    newCfgFile << std::setw(4) << cfg;
	    newCfgFile.close();

	    if (chip_config_filename != "") filename = chip_config_filename + "/" + filename;

	    json jchipconnectivity;
	    jchipconnectivity["config"] = filename;
	    jchipconnectivity["path"] = path_relation;
	    jchipconnectivity["tx"] = _tx;
	    jchipconnectivity["rx"] = _rx;
	    jchipconnectivity["enable"] = 1;
	    jchipconnectivity["locked"] = 0;

	    jconnectivity["chips"].push_back(jchipconnectivity);

	    hw->disableRx();
	    }
	}

	std::ofstream newConnectivityFile(connectivity_filename);
	newConnectivityFile << std::setw(4) << jconnectivity;
	newConnectivityFile.close();
	logger->info("Connectivity file saved in \"{}\".", connectivity_filename);
	logger->info("Chip configs saved in \"{}\".", chip_config_path);
	return 0;
}
