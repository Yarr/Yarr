#include <iostream>
#include <iomanip>
#include <fstream>

// This is where getopt is defined
#include <unistd.h>

#include "AllHwControllers.h"
#include "AllChips.h"
#include "Bookkeeper.h"
#include "ScanFactory.h"

#include "storage.hpp"

// By default, don't talk to controllers
bool config_controllers = false;
bool verbose = false;

enum class ConfigType {
  CONNECTIVITY,
  CONTROLLER,
  FRONT_END,
  SCAN_CONFIG,
  UNKNOWN
};

std::ostream &operator <<(std::ostream &os, ConfigType &ct) {
  switch(ct) {
  case ConfigType::CONNECTIVITY: os << "CONNECTIVITY"; break;
  case ConfigType::CONTROLLER: os << "CONTROLLER"; break;
  case ConfigType::FRONT_END: os << "FRONT_END"; break;
  case ConfigType::SCAN_CONFIG: os << "SCAN_CONFIG"; break;
  default: os << "UNKNOWN"; break;
  }
  return os;
}

ConfigType fromString(std::string s) {
  if(s == "CONNECTIVITY") return ConfigType::CONNECTIVITY;
  if(s == "CONTROLLER") return ConfigType::CONTROLLER;
  if(s == "FRONT_END") return ConfigType::FRONT_END;
  if(s == "SCAN_CONFIG") return ConfigType::SCAN_CONFIG;
  return ConfigType::UNKNOWN;
}

bool testController(json controller_file) {
  if(!config_controllers) {
    std::cout << "Skip checking controller config file\n";
    return true;
  }

  try {
    // Top-level name
    auto ctrl = controller_file["ctrlCfg"];

    auto hwCtrl = StdDict::getHwController(ctrl["type"]);
    hwCtrl->loadConfig(ctrl["cfg"]);

    return true;
  } catch(std::runtime_error &e) {
    std::cout << "Controller read failed: " << e.what() << "\n";
    return false;
  }
}

bool testConnectivity(json config) {
  try {
    if (config["chipType"].empty() || config["chips"].empty()) {
      std::cout << "chip type or chips not specified!\n";
      return false;
    }

    auto chipType = config["chipType"];

    // Loop over chips
    for (unsigned i=0; i<config["chips"].size(); i++) {
      json chip = config["chips"][i];
      std::string chipConfigPath = chip["config"];

      std::shared_ptr<FrontEnd> fe = StdDict::getFrontEnd(chipType);
      chip["tx"]; // int
      chip["rx"]; // int

#if 0
      /* Skip checking of configure fields, as not always there? */
      std::ifstream cfgFile(chipConfigPath);
      if (!cfgFile) {
        std::cout << "Failed to read " << chipConfigPath << "\n";
        // Uses default, don't care about json?
        return false;
      }

      //json cfg = json::parse(cfgFile);
      FrontEndCfg *feCfg = dynamic_cast<FrontEndCfg*>(&*fe);
      feCfg->fromFileJson(cfg);

      if (!chip["locked"].empty())
        feCfg->setLocked(chip["locked"]);

      cfgFile.close();
#endif
    }
  } catch(std::runtime_error &te) {
    std::cout << "Connectivity read failed: " << te.what() << "\n";
    return false;
  }

  return true;
}

bool testScanConfig(json config) {
  try {
    Bookkeeper b{0, 0};
    ScanFactory s(&b);
    s.loadConfig(config);

    {
      json histo = config["scan"]["histogrammer"];

      int n = histo["n_count"];
      for(int i=0; i<n; i++) {
        std::string algo_name = histo[std::to_string(i)]["algorithm"];
      }
    }

    {
      json analysis = config["scan"]["analysis"];

      int n = analysis["n_count"];
      for(int i=0; i<n; i++) {
        std::string algo_name = analysis[std::to_string(i)]["algorithm"];
      }
    }

    return true;
  } catch(std::runtime_error &te) {
    std::cout << "Scan config read failed: " << te.what() << "\n";
    return false;
  }
}

int checkJson(json &jsonConfig, ConfigType jsonFileType) {
  switch(jsonFileType) {
  case ConfigType::FRONT_END:
    {
      int count = 0;
      std::string name;
      for(auto &el: jsonConfig) {
        name = el;
        count ++;
      }
      if(count != 1) {
        std::cout << "Expect one top-level entry in FrontEnd config\n";
        return 1;
      }

      if(name == "FE65-P2") name = "FE65P2";
      else if(name == "FE-I4B") name = "FEI4B";

      auto fe = std::move(StdDict::getFrontEnd(name));
      if(!fe) {
        std::cout << "FrontEnd not found: " << name << "!\n";
        return 2;
      }
      FrontEndCfg *cfg = dynamic_cast<FrontEndCfg*>(fe.get());
      if(cfg == nullptr) {
        std::cout << "FrontEnd: " << name << " does not implement FrontEndCfg!\n";
        return 2;
      }

      cfg->fromFileJson(jsonConfig);
    }
    break;
  case ConfigType::CONNECTIVITY:
    if(!testConnectivity(jsonConfig)) {
      std::cout << "Failed to parse as connectivity\n";
    }
    break;
  case ConfigType::SCAN_CONFIG:
    if(!testScanConfig(jsonConfig)) {
      std::cout << "Failed to parse as scan config\n";
    }
    break;
  case ConfigType::CONTROLLER:
    if(!testController(jsonConfig)) {
      std::cout << "Failed to parse as controller config\n";
    }
    break;
  default:
    std::cout << "Unspecified file type, top-level names are:\n";

    for (auto& el : jsonConfig) {
        std::cout << "  " << el << '\n';
    }
    break;
  }
  return 0;
}

void printHelp() {
    std::cout << "testJson help:" << std::endl;
    std::cout << "  Read in json configuration files to check parsing\n";
    std::cout << "\n";
    std::cout << "Parameters\n";
    std::cout << " -h: Shows this help\n";
    std::cout << " -f <config_name> : Json file to examine\n";
    std::cout << "                   CONNECTIVITY,CONTROLLER,SCAN_CONFIG,FRONT_END\n";
    std::cout << " -t <config_type> : Type of config file, default to guessing\n";
    std::cout << " -K : Check controller file, which might talk to hardware\n";
    std::cout << " -v : Be more verbose\n";
}

int main(int argc, char *argv[]) {
  // Default parameters
  std::string jsonFileName = "";

  ConfigType jsonFileType = ConfigType::UNKNOWN;

  int c;
  while ((c = getopt(argc, argv, "hvKf:t:")) != -1) {
    switch (c) {
    case 'h':
      printHelp();
      return 0;
    case 'v':
      verbose = true;
      break;
    case 'K':
      config_controllers = true;
      break;
    case 'f':
      jsonFileName = std::string(optarg);
      break;
    case 't':
      jsonFileType = fromString(optarg);
      if(verbose)
        std::cout << "Interpreting type parameter as: " << jsonFileType << '\n';
      break;
    case '?':
      if(optopt == 'f' || optopt == 't'){
        std::cerr << "-> Option " << (char)optopt
                  << " requires a parameter! (aborting)\n";
      } else {
        std::cerr << "-> Unknown parameter: " << (char)optopt << '\n';
      }
      return -1;
    default:
      std::cerr << "-> Error while parsing command line parameters!\n";
      return -1;
    }
  }

  if (optind != argc) {
    std::cerr << "Failed to parse all cmdline parameters " << (argc-optind) << " remaining\n";
    printHelp();
    return -1;
  }

  if(jsonFileName.empty()) {
    std::cerr << "Need at least a file name\n";
    printHelp();
    return -1;
  } else {
    if(verbose)
      std::cout << "Parsing file: " << jsonFileName << '\n';
  }

  json jsonConfig;
  try {
    std::ifstream ifs(jsonFileName);
    if(!ifs) {
      std::cerr << "Failed to open file: " << jsonFileName << '\n';
      return 1;
    }
    jsonConfig = json::parse(ifs);
    ifs.close();
  } catch (std::runtime_error &e) {
    std::cerr << "Failed to parse file as json: " << e.what() << '\n';
    // file_name
    return 1;
  }

  try {
    int ret = checkJson(jsonConfig, jsonFileType);
    if(ret != 0) {
      std::cerr << "Failed to interpret config file\n";
      return ret;
    }
  } catch (std::runtime_error &e) {
    std::cerr << "Failed to interpret config file: " << e.what() << '\n';
    return 2;
  }

  if(verbose)
    std::cout << "Successfully interpreted file: " << jsonFileName << " as " << jsonFileType << '\n';

  return 0;
}
