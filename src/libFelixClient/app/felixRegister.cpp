#include "FelixController.h"
#include "logging.h"
#include "LoggingConfig.h"
#include "ScanHelper.h"
#include "ScanOpts.h"

#include <iostream>

namespace {
  auto logger = logging::make_log("felixRegisters");

  void printHelp() {
    std::cout << "Usage: felixRgister HW_CONFIG REGISTER_NAME [REGISTER_VALUE] [OPTIONS...]" << std::endl;
    std::cout << "  Read or write FELIX registers via the FelixController" << std::endl;
    std::cout << " -h : Show this help." << std::endl;
    std::cout << " -l LOG_CONFIG : Configuration for the logger." << std::endl;
    std::cout << "Examples:" << std::endl;
    std::cout << "* To read the FELIX register FIRMWARE_MODE:" << std::endl;
    std::cout << "  bin/felixRegister configs/controller/felix_client.json FIRMWARE_MODE" << std::endl;
    std::cout << "* To write 0xabcd to FELIX register BROADCAST_ENABLE_00:" << std::endl;
    std::cout << "  bin/felixRegister configs/controller/felix_client.json BROADCAST_ENABLE_00 0xabcd" << std::endl;
  }
}

int main(int argc, char **argv) {

  std::string logCfg;

  int opt;
  while ((opt = getopt(argc, argv, "hl:")) != -1) {
    switch(opt) {
    case 'h':
      printHelp();
      return 0;
    case 'l':
      logCfg = std::string(optarg);
      break;
    default:
      spdlog::critical("Error while parsing command line parameters!");
      return -1;
    }
  }

  if (argc < 3) {
    printHelp();
    return 1;
  }

  // First positional parameter
  std::string ctrlCfg = argv[optind];

  // Second positional parameter
  std::string regName = argv[optind+1];

  std::string regValue_str;
  if (argc > 3) {
    regValue_str = argv[optind+2];
  }

  // Configure logger
  if (logCfg.empty()) { // default
    ScanOpts options;
    json jlog;
    jlog["pattern"] = options.defaultLogPattern;
    jlog["log_config"][0]["name"] = "all";
    jlog["log_config"][0]["level"] = "info";
    logging::setupLoggers(jlog);
  } else {
    try {
      auto jlog = ScanHelper::openJsonFile(logCfg);
      logging::setupLoggers(jlog);
    } catch (std::runtime_error &e) {
      spdlog::error("Failed to load logger config: {}", e.what());
      return -1;
    }
  }

  // Configure controller
  json jctrl;
  try {
    jctrl = ScanHelper::openJsonFile(ctrlCfg);
    if (jctrl["ctrlCfg"]["type"] != "FelixClient") {
      logger->critical("The controller type is not FelixClient.");
      return -1;
    }
  } catch (std::runtime_error &e) {
    logger->critical("Cannot open controller config: {}", e.what());
    return -1;
  }

  auto hwCtrl = std::make_unique<FelixController>();

  try {
    hwCtrl->loadConfig(jctrl["ctrlCfg"]["cfg"]);
  } catch (std::runtime_error &e) {
    logger->error("Failed to load controller config: {}", e.what());
    return -1;
  }

  uint64_t regValue;

  if (regValue_str.empty()) {
    // Register read
    if (hwCtrl->readFelixRegister(regName, regValue)) {
      std::cout << std::endl;
      std::cout << regName << " = 0x" << std::hex << regValue << std::endl;
      std::cout << std::endl;
    }
  } else {
    // Register write
    hwCtrl->writeFelixRegister(regName, regValue_str);
  }

  return 0;
}