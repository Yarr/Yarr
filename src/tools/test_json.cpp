#include <iostream>
#include <iomanip>
#include <fstream>

// This is where getopt is defined
#include <unistd.h>

#include "AllHwControllers.h"
#include "AllChips.h"
#include "Bookkeeper.h"
#include "AnalysisDataProcessor.h"
#include "HistoDataProcessor.h"
#include "ScanFactory.h"
#include "ScanHelper.h"

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

/// Minimal FrontEnd needed, otherwise scan config not checked
class MyFrontEnd : public FrontEnd {
public:
  void init(HwController *arg_core, const FrontEndConnectivity& fe_cfg) {}
  void maskPixel(unsigned col, unsigned row) {}
  unsigned getPixelEn(unsigned col, unsigned row) { return 1; }
  void enableAll() {}
  void configure() {}
  void writeNamedRegister(std::string name, uint16_t value) {}
  void setInjCharge(double, bool, bool) {}
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
    if (!config.contains("chipType") || !config.contains("chips")) {
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

      if (chip.contains("locked"))
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

bool testScanConfig(const json &scanConfig) {
  try {
    Bookkeeper b{nullptr, nullptr};
    FeedbackClipboardMap* feedbackMap = nullptr;
    ScanFactory s(&b, feedbackMap);
    s.loadConfig(scanConfig);

    // Using ScanHelper for consistency, but that needs more infrastructure
    std::map<unsigned, std::unique_ptr<HistoDataProcessor> > histogrammers;
    std::map<unsigned, std::vector<std::unique_ptr<AnalysisDataProcessor>> > analyses;

    std::unique_ptr<FrontEnd> fe(new MyFrontEnd());

    // If we don't add a front-end the info isn't checked...
    b.addFe(fe.release(), FrontEndConnectivity(12,12));

    // This is run by ScanHelper, but doesn't depend on config
    // ScanHelper::buildRawDataProcs(procs, bookie, chipType);
    ScanHelper::buildHistogrammers(histogrammers, scanConfig,
                                   b, "no_dir");
    int mask_opt = 0;
    ScanHelper::buildAnalyses(analyses, scanConfig, b, &s,
                              feedbackMap, mask_opt, "no_dir", 
                              -1, -1);

    return true;
  } catch(std::runtime_error &te) {
    std::cout << "Scan config read failed: " << te.what() << "\n";
    return false;
  }
}

int checkJsonFE(json &jsonConfig, std::string fe_name) {
  auto fe = std::move(StdDict::getFrontEnd(fe_name));
  if(!fe) {
    std::cout << "FrontEnd not found: " << fe_name << "!\n";
    return 2;
  }
  FrontEndCfg *cfg = dynamic_cast<FrontEndCfg*>(fe.get());
  if(cfg == nullptr) {
    std::cout << "FrontEnd: " << fe_name << " does not implement FrontEndCfg!\n";
    return 2;
  }

  cfg->loadConfig(jsonConfig);
  return 0;
}

int checkJsonFE(json &jsonConfig) {
  int count = 0;
  std::string name;
  {
    auto b = std::begin(jsonConfig);
    auto e = std::end(jsonConfig);
    for(auto i=b; i!=e; i++) {
      name = i.key();
      count ++;
    }
  }
  if(count != 1) {
    if(jsonConfig.contains("HCC") && jsonConfig.contains("ABCs")) {
      bool pass_all = true;
      bool pass_one = false;
      for(auto &n: {"Star", "Star_vH0A1", "Star_vH1A1"}) {
        try {
          int ret_val = checkJsonFE(jsonConfig, n);
          if(ret_val != 0) {
            // Failure to load class, nothing to do with the file
            return ret_val;
          }
          pass_one = true;
        } catch (std::out_of_range &e) {
          pass_all = false;
        }
      }
      if(pass_all) {
        std::cout << "Loaded successfully in all Star variations\n";
      }
      if(pass_one) {
        return 0;
      } else {
        return 1;
      }
    } else {
      std::cout << "Expect one top-level entry (not " << count << ") in FrontEnd config (or it's Star)\n";

      auto b = std::begin(jsonConfig);
      auto e = std::end(jsonConfig);
      for(auto i=b; i!=e; i++) {
        std::cout << "  " << i.key() << "\n";
      }
      return 1;
    }
  }

  if(name == "FE65-P2") name = "FE65P2";
  else if(name == "FE-I4B") name = "FEI4B";

  return checkJsonFE(jsonConfig, name);
}

int checkJson(json &jsonConfig, ConfigType jsonFileType) {
  switch(jsonFileType) {
  case ConfigType::FRONT_END:
    return checkJsonFE(jsonConfig);
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

    {
      auto b = std::begin(jsonConfig);
      auto e = std::end(jsonConfig);
      for(auto i=b; i!=e; i++) {
        std::cout << "  " << i.key() << '\n';
      }
    }
    break;
  }
  return 0;
}

void printHelp() {
    std::cout << "test_json help:" << std::endl;
    std::cout << "  Read in json configuration files to check parsing\n";
    std::cout << "\n";
    std::cout << "Parameters\n";
    std::cout << " -h: Shows this help\n";
    std::cout << " -f <config_name> : Json file to examine\n";
    std::cout << " -t <config_type> : Type of config file, default to guessing\n";
    std::cout << "                   CONNECTIVITY,CONTROLLER,SCAN_CONFIG,FRONT_END\n";
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
    if(jsonConfig.is_null()) {
      std::cerr << "Failed to parse file: " << jsonFileName << '\n';
      return 1;
    }
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
