#include <iostream>
#include <fstream>

// This is where getopt is defined
#include <unistd.h>

#include "AllHwControllers.h"
#include "AllChips.h"
#include "AllProcessors.h"

#include "json.hpp"

using json=nlohmann::basic_json<std::map, std::vector, std::string, bool, int32_t, uint32_t, float>;

enum class ConfigType {
  FRONT_END,
  UNKNOWN
};

bool verbose = false;

std::ostream &operator <<(std::ostream &os, ConfigType &ct) {
  switch(ct) {
  case ConfigType::FRONT_END: os << "FRONT_END"; break;
  default: os << "UNKNOWN"; break;
  }
  return os;
}

ConfigType fromString(std::string s) {
  if(s == "FRONT_END") return ConfigType::FRONT_END;
  return ConfigType::UNKNOWN;
}

int checkJson(json &jsonConfig, ConfigType jsonFileType) {
  switch(jsonFileType) {
  case ConfigType::FRONT_END:
    {
      int count = 0;
      std::string name;
      for(auto &el: jsonConfig.items()) {
        name = el.key();
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
  default:
    std::cout << "Top-level names are:\n";

    for (auto& el : jsonConfig.items()) {
        std::cout << "  " << el.key() << '\n';
    }
    break;
  }
  return 0;
}

void printHelp() {
    std::cout << "testJson help:" << std::endl;
    std::cout << " -h: Shows this help\n";
    std::cout << " -f <config_name> : Json file to examine\n";
    std::cout << " -t <config_type> : Type of config file, default to guessing\n";
    std::cout << " -v : Be more verbose\n";
}

int main(int argc, char *argv[]) {
  // Default parameters
  std::string jsonFileName = "";

  ConfigType jsonFileType = ConfigType::UNKNOWN;

  int c;
  while ((c = getopt(argc, argv, "hvf:t:")) != -1) {
    switch (c) {
    case 'h':
      printHelp();
      return 0;
    case 'v':
      verbose = true;
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
    jsonConfig = json::parse(std::ifstream(jsonFileName));
  } catch (json::parse_error &e) {
    std::cerr << "Failed to parse file as json: " << e.what() << '\n';
    return 1;
  }

  try {
    int ret = checkJson(jsonConfig, jsonFileType);
    if(ret != 0) {
      std::cerr << "Failed to interpret config file\n";
      return ret;
    }
  } catch (json::type_error &e) {
    std::cerr << "Failed to interpret config file: " << e.what() << '\n';
    return 2;
  }

  if(verbose)
    std::cout << "Successfully interpreted file: " << jsonFileName << " as " << jsonFileType << '\n';

  return 0;
}
