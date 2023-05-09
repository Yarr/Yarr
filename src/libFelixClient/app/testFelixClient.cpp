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
  std::cout << " -t <TX_ELINK1> [<TX_ELINK2> ...] : A list of tx elinks for sending data." << std::endl;
  std::cout << " -r <RX_ELINK1> [<RX_ELINK2> ...] : A list of rx elinks for receiving data." << std::endl;
  std::cout << " -l LOG_CONFIG : Configuration for the logger." << std::endl;
}

int main(int argc, char **argv) {

  std::string controllerCfg;
  std::string loggerCfg;
  std::vector<unsigned> elinks_tx;
  std::vector<unsigned> elinks_rx;

  int opt;
  while ((opt = getopt(argc, argv, "ht:r:l:")) != -1) {
    switch(opt) {
    case 'h':
      printHelp();
      return 0;
    case 't':
      elinks_tx.clear();
      optind -= 1;
      for (; optind < argc && *argv[optind] != '-'; optind += 1) {
        elinks_tx.push_back( atoi(argv[optind]) );
      }
       break;
    case 'r':
      elinks_rx.clear();
      optind -= 1;
      for (; optind < argc && *argv[optind] != '-'; optind += 1) {
        elinks_rx.push_back( atoi(argv[optind]) );
      }
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

  // Subscribe to elinks
  if (not elinks_rx.empty()) {
    try {
      hwCtrl->setRxEnable(elinks_rx);
    } catch (std::runtime_error& e) {
      logger->error("Fail to subscribe: {}", e.what());
      return 1;
    }

    dynamic_cast<FelixController*>(hwCtrl.get())->runMonitor(true);
  }

  // Send data
  if (not elinks_tx.empty()) {
    try {
      // Read FELIX registers
      json j_status = hwCtrl->getStatus();
      j_status.dump();
    } catch (std::runtime_error& e) {
      logger->warn("Cannot read FELIX registers");
    }

    try {
      hwCtrl->setCmdEnable(elinks_tx);

      logger->info("Write 0xdeadbeef to enabled tx elinks");
      hwCtrl->writeFifo(0xdeadbeef);
      hwCtrl->releaseFifo();

      logger->info("Send triggers");
      uint32_t trigWords[2] = {0x7259cafe, 0x89abcdef};
      hwCtrl->setTrigWord(trigWords, 2);
      hwCtrl->setTrigFreq(10);
      hwCtrl->setTrigCnt(5);
      hwCtrl->setTrigConfig(INT_COUNT);

      hwCtrl->setTrigEnable(1);
      while (not hwCtrl->isTrigDone());
      hwCtrl->setTrigEnable(0);

    } catch (std::runtime_error& e) {
      logger->error("Fail to send data: {}", e.what());
      return 1;
    }
  }

  // Collect and report the received data
  if (not elinks_rx.empty()) {
    // Wait for data
    logger->info("Waiting for data...");
    std::this_thread::sleep_for(std::chrono::seconds(5));

    dynamic_cast<FelixController*>(hwCtrl.get())->stopMonitor();

    hwCtrl->disableRx();

    RawDataContainer rdc({});
    while (hwCtrl->getCurCount()) {
      auto dataVec = hwCtrl->readData();
      for(auto data : dataVec) {
        rdc.add(data);
      }
    }

    logger->info("Received {} data", rdc.size());
    logger->info("Rate: {} B/s", hwCtrl->getDataRate());

    for (auto rdp : rdc.data) {
      std::stringstream ss;
      ss << " " << rdp->getAdr() << " " << rdp->getBuf();
      ss << " " << rdp->getSize() << " : " << std::hex;

      for (unsigned j=0; j<rdp->getSize(); ++j) {
        ss << "0x" << rdp->get(j) << " ";
      }
       logger->debug(" {}", ss.str());
    }
  }

  return 0;
}
