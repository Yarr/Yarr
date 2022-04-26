#include "AllHwControllers.h"
#include "HwController.h"
#include "FelixController.h"
#include "logging.h"
#include "LoggingConfig.h"
#include "ScanHelper.h"
#include "storage.hpp"

#include <iostream>
#include <sstream>

auto logger = logging::make_log("testFelixClient");

void printHelp() {
  std::cout << "Usage: testFelixClient HW_CONFIG [OPTIONS...]" << std::endl;
  std::cout << "  Test FelixClient controller" << std::endl;
  std::cout << " -h : Show this help." << std::endl;
  std::cout << " -t ELINK : Tx elink for sending data." << std::endl;
  std::cout << " -r ELINK : Rx elink for receiving data." << std::endl;
  std::cout << " -l LOG_CONFIG : Configuration for the logger." << std::endl;
}

int main(int argc, char **argv) {

  std::string controllerCfg;
  std::string loggerCfg;
  int elink_tx = -1;
  int elink_rx = -1;

  int opt;
  while ((opt = getopt(argc, argv, "ht:r:l:")) != -1) {
    switch(opt) {
    case 'h':
      printHelp();
      return 0;
    case 't':
      elink_tx = atoi(optarg);
      break;
    case 'r':
      elink_rx = atoi(optarg);
      break;
    case 'l':
      loggerCfg = std::string(optarg);
      break;
    default:
      spdlog::critical("Error while parsing command line parameters!");
      return -1;
    }
  }

  // First positional parameter
  if (optind != argc) {
    controllerCfg = argv[optind];
  }

  spdlog::info("Configuring logger ...");
  if (loggerCfg.empty()) {
    // default
    json jlog;
    jlog["pattern"] = "[%T:%e]%^[%=8l][%=15n]:%$ %v";
    jlog["log_config"][0]["name"] = "testFelixClient";
    jlog["log_config"][0]["level"] = "info";
    jlog["log_config"][1]["name"] = "FelixTxCore";
    jlog["log_config"][1]["level"] = "info";
    jlog["log_config"][2]["name"] = "FelixRxCore";
    jlog["log_config"][2]["level"] = "info";
    logging::setupLoggers(jlog);
  } else {
    try {
      auto jlog = ScanHelper::openJsonFile(loggerCfg);
      logging::setupLoggers(jlog);
    } catch (std::runtime_error &e) {
      spdlog::error("Failed to load logger config: {}", e.what());
      return -1;
    }
  }

  // controller config
  json ctrlCfg;
  try {
    ctrlCfg = ScanHelper::openJsonFile(controllerCfg);
  } catch (std::runtime_error &e) {
    logger->critical("Cannot open controller config: {}", e.what());
    return -1;
  }

  if (ctrlCfg["ctrlCfg"]["type"] != "FelixClient") {
    logger->critical("The controller type is not FelixClient.");
    return -1;
  }

  std::unique_ptr<HwController> hwCtrl = StdDict::getHwController("FelixClient");

  try {
    hwCtrl->loadConfig(ctrlCfg["ctrlCfg"]["cfg"]);
  } catch (std::runtime_error &e) {
    logger->error("Failed to load controller config: {}", e.what());
    return -1;
  }

  // Send data
  if (elink_tx >= 0) {
    try {
      logger->info("Sending data to chn {}", elink_tx);
      hwCtrl->setCmdEnable(elink_tx);

      hwCtrl->writeFifo(0xdeadbeef);
      hwCtrl->releaseFifo();

      // triggers
      uint32_t trigWords[2] = {0x01234567, 0x89abcdef};
      hwCtrl->setTrigWord(trigWords, 2);
      hwCtrl->setTrigFreq(1);
      hwCtrl->setTrigCnt(5);
      hwCtrl->setTrigConfig(INT_COUNT);

      hwCtrl->setTrigEnable(1);
      while (not hwCtrl->isTrigDone());
      hwCtrl->setTrigEnable(0);

    } catch (std::runtime_error& e) {
      logger->error("Failed to send: {}", e.what());
    }
  }

  // Receive data
  if (elink_rx >= 0) {
    try {
      hwCtrl->setRxEnable(elink_rx);

      // start monitoring
      dynamic_cast<FelixController*>(hwCtrl.get())->runMonitor(true);

      // wait for data
      logger->info("Waiting for data...");
      std::this_thread::sleep_for(std::chrono::seconds(5));

      dynamic_cast<FelixController*>(hwCtrl.get())->stopMonitor();

      hwCtrl->disableRx();

      // Collect and report data
      RawDataContainer rdc({});
      while (hwCtrl->getCurCount()) {
        auto rd = hwCtrl->readData();
        if (rd) {
          rdc.add(rd);
        }
      }

      logger->info("Received {} data", rdc.size());
      logger->info("Rate: {} B/s", hwCtrl->getDataRate());

      for (unsigned i=0; i<rdc.size(); i++) {
        std::stringstream ss;
        ss << " " << rdc.adr[i] << " " << rdc.buf[i] << " " << rdc.words[i];
        ss << " : " << std::hex;
        for (unsigned j=0; j<rdc.words[i]; j++) {
          ss << "0x" << rdc.buf[i][j] << " ";
        }
        logger->debug(" {}", ss.str());
      }

    } catch (std::runtime_error& e) {
      logger->error("Failed to subscribe: {}", e.what());
    }
  }

  return 0;
}
