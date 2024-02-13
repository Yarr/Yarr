#include "AllHwControllers.h"
#include "HwController.h"
#include "FelixController.h"
#include "logging.h"
#include "LoggingConfig.h"
#include "ScanHelper.h"
#include "storage.hpp"

#include <iostream>
#include <sstream>
#include <fstream>
#include <string>
#include <cstdlib>

auto logger = logging::make_log("testFelixClient");

void printHelp() {
  std::cout << "Usage: testFelixClient HW_CONFIG [OPTIONS...]" << std::endl;
  std::cout << "  Test FelixClient controller" << std::endl;
  std::cout << " -h : Show this help." << std::endl;
  std::cout << std::endl;
  std::cout << " -t <TX_ELINK1> [<TX_ELINK2> ...] : A list of tx elinks for sending data." << std::endl;
  std::cout << " -d <32b HEX WORD> [<32b HEX WORD> ...] : A list of data words in hex format to be sent." << std::endl;
  std::cout << " -f <FILE_NAME> : Name of a file containing data words to be sent. It is expected each line has one 32-bit hex integer." << std::endl;
  std::cout << " -n NUMBER : The number of times to send the data. Default: 1" << std::endl;
  std::cout << " -q FREQUENCY : Trigger frequency in Hz. If non-zero, send the data using TxCore::trigger(). Otherwise, send the data using TxCore::releaseFifo(). Default: 0" << std::endl;
  std::cout << std::endl;
  std::cout << " -r <RX_ELINK1> [<RX_ELINK2> ...] : A list of rx elinks for receiving data." << std::endl;
  std::cout << " -w SECONDS : Number of seconds to wait for data. Default: 1" << std::endl;
  std::cout << std::endl;
  std::cout << " -s : Read FELIX status registers" << std::endl;
  std::cout << " -l LOG_CONFIG : Configuration for the logger." << std::endl;
  std::cout << std::endl;
  std::cout << "Examples:" << std::endl;
  std::cout << "* To read FELIX status registers: bin/testFelixClient configs/controller/felix_client.json -s" << std::endl;
  std::cout << "* To send some data e.g. 0xdeadbeef to elinks 1, 6, 11, and 16 twice: bin/testFelixClient configs/controller/felix_client.json -t 1 6 11 16 -d 0xdeadbeef -n 2" << std::endl;
  std::cout << "* To receive data from elinks 0 and 2 for 5 seconds: bin/testFelixClient configs/controller/felix_client.json -r 0 2 -w 5" << std::endl;
}

int main(int argc, char **argv) {

  std::string controllerCfg;
  std::string loggerCfg;

  bool readFelixStatus = false;

  std::vector<unsigned> elinks_tx;
  std::vector<unsigned> data_tx;
  std::string file_data_tx;
  unsigned nrepetitions = 1;
  double trigFreq = 0;

  std::vector<unsigned> elinks_rx;
  unsigned waitTime = 1; // second

  int opt;
  while ((opt = getopt(argc, argv, "ht:d:f:n:q:r:w:sl:")) != -1) {
    switch(opt) {
    case 'h':
      printHelp();
      return 0;
    case 't':
      elinks_tx.clear();
      optind -= 1;
      for (; optind < argc && *argv[optind] != '-'; optind += 1) {
        elinks_tx.push_back( std::atoi(argv[optind]) );
      }
      break;
    case 'd':
      optind -= 1;
      for (; optind < argc && *argv[optind] != '-'; optind += 1) {
        data_tx.push_back( std::stoul(std::string(argv[optind]), nullptr, 16) );
      }
      break;
    case 'f':
      file_data_tx = std::string(optarg);
      break;
    case 'n':
      nrepetitions = std::atoi(optarg);
      break;
    case 'q':
      trigFreq = std::stod(optarg);
      break;
    case 'r':
      elinks_rx.clear();
      optind -= 1;
      for (; optind < argc && *argv[optind] != '-'; optind += 1) {
        elinks_rx.push_back( std::atoi(argv[optind]) );
      }
      break;
    case 'w':
      waitTime = std::atoi(optarg);
      break;
    case 's':
      readFelixStatus = true;
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

  // Read FELIX registers
  if (readFelixStatus) {
    try {
      json j_status = hwCtrl->getStatus();
      j_status.dump();
    } catch (std::runtime_error& e) {
      logger->warn("Cannot read FELIX registers");
    }
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

    if (not file_data_tx.empty()) {
      // read data from a file
      std::ifstream datafile(file_data_tx);
      if (datafile.is_open()) {
        std::string line;
        while(std::getline(datafile, line)) {
          data_tx.push_back( std::stoul(line, nullptr, 16) );
        }
        datafile.close();
      } else {
        logger->error("Cannot open data file {}", file_data_tx);
      }
    }

    try {
      hwCtrl->setCmdEnable(elinks_tx);

      logger->info("Words to be sent to the enabled tx elinks {} times:", nrepetitions);
      for (unsigned word : data_tx) {
        logger->info(" 0x{:x}", word);
      }

      if (trigFreq) {
        logger->info("Sending the words as triggers");
        hwCtrl->setTrigWord(data_tx.data(), data_tx.size());
        hwCtrl->setTrigFreq(trigFreq);
        hwCtrl->setTrigCnt(nrepetitions);
        hwCtrl->setTrigConfig(INT_COUNT);

        hwCtrl->setTrigEnable(1);
        while (not hwCtrl->isTrigDone());
        hwCtrl->setTrigEnable(0);

      } else {
        logger->info("Sending the words");
        for (unsigned i=0; i<nrepetitions; i++) {
          for (const auto& word : data_tx) {
            hwCtrl->writeFifo(word);
          }
          hwCtrl->releaseFifo();
        }
      }
    } catch (std::runtime_error& e) {
      logger->error("Fail to send data: {}", e.what());
      return 1;
    }
  }

  // Collect and report the received data
  if (not elinks_rx.empty()) {
    // Wait for data
    logger->info("Waiting for data...");
    std::this_thread::sleep_for(std::chrono::seconds(waitTime));

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
