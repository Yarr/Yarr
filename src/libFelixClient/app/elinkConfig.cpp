#include <getopt.h>
#include <memory>
#include <set>

#include "logging.h"
#include "LoggingConfig.h"
#include "ScanHelper.h"
#include "AllHwControllers.h"
#include "FelixController.h"

namespace {
  auto logger = logging::make_log("elinkConfig");

  void printHelp() {
    std::cout << "A program to configure E-Links" << std::endl;
    std::cout << "Usage: elinkConfig COMMAND HW_CONFIG [OPTIONS...]" << std::endl;
    std::cout << "Commands:" << std::endl;
    std::cout << " get : Check if all e-links specified in options are enabled." << std::endl;
    std::cout << " set : Enable all e-links specified in options." << std::endl;
    std::cout << " off : Disable all e-links." << std::endl;
    std::cout << "Options:" << std::endl;
    std::cout << " -h : Show this help." << std::endl;
    std::cout << " -t TX_CHANNELS : A list of Tx channels to be configured." << std::endl;
    std::cout << " -r RX_CHANNELS : A list of Rx channels to be configured." << std::endl;
    std::cout << " -c CHIP_CONFIG : Connectivity configuration file." << std::endl;
    std::cout << " -b BANDWIDTH : Rx bandwidth in Mbps. Overrides the value if specified in the \"Card\" field of the controller file" << std::endl;
    std::cout << " -e : Channels specifeid in options are enabled exclusively. All other channels are disabled." << std::endl;
    std::cout << " -I : Include IC channels" << std::endl;
    std::cout << " -E : Include EC channels" << std::endl;
    std::cout << " --ED : Include path encoding/decoding" << std::endl;
    std::cout << " --tx-encoding ENCODING_VALUE : Value of the tx encoding type. Overrides the value if specified in the \"Card\" field of the controller file." << std::endl;
    std::cout << " --rx-decoding DECODING_VALUE : Value of the rx decoding type. Overrides the value if specified in the \"Card\" field of the controller file." << std::endl;
    std::cout << " -l LOG_CONFIG : Configuration for the logger." << std::endl;
    std::cout << " -v : Verbose mode. Set logging level to 'debug'. Overwritten by '-l LOG_CONFIG' if a logging configuration is provided." << std::endl;
    std::cout << "Examples:" << std::endl;
    std::cout << "* To check if all elinks corresponding to the Rx channels 0, 2, 4, 6, 8, and 10 are enabled and configured with a bandwidth of 640 Mbps:" << std::endl;
    std::cout << "  bin/elinkConfig get configs/controller/felix_client.json -r 0 2 4 6 8 10 -b 640" << std::endl;
    std::cout << "* To enable exclusively the elinks specified in a connectivity config as well as the relevant IC and EC channels, and disable all other elinks:" << std::endl;
    std::cout << "  bin/elinkConfig set configs/controller/felix_client.json -c <connectivity.json> -e -I -E" << std::endl;
    std::cout << "* To turn off all elinks in a connectivity config:" << std::endl;
    std::cout << "  bin/elinkConfig off -c <connectivity.json>" << std::endl;
    std::cout << "* To set the encoding and decoding patterns for the specified Tx and Rx channels:" << std::endl;
    std::cout << "  bin/elinkConfig set configs/controller/felix_client.json -t 0 1 2 3 -r 0 2 4 6 --ED --tx-encoding 4 --rx-decoding 3" << std::endl;
    std::cout << "* Can also specify the rx/tx channels via the connectivity file" << std::endl;
    std::cout << "  bin/elinkConfig set configs/controller/felix_client.json -c <connectivity.json> --ED --tx-encoding 4 --rx-decoding 3" << std::endl;

    //std::cout << " -L LINK_NUMBERS :  A list of link numbers for considering other channels that are not specified via -t, -r, or -c. Default is including all 12 links on a logical FLX device."
  }

  void checkICEnable(FelixController* flx, const std::vector<FelixTools::FelixID_t>& fids, const std::string& label, bool exclusive) {
    if (fids.empty()) return;

    logger->info("Checking IC ({}) enable registers...", label);

    if (exclusive) {
      if ( flx->getICEnableExclusive(fids) ) {
        logger->info(" All relevant IC ({}) channeles are enabled and all others are disabled!", label);
      } else {
        logger->warn(" Not all relevant IC ({}) channels are enabled or not all other IC channels are disabled!", label);
      }
    } else {
      if ( flx->getICEnable(fids) ) {
        logger->info(" All relevant IC ({}) channeles are enabled!", label);
      } else {
        logger->warn(" Not all relevant IC ({}) channels are enabled!", label);
      }
    }
  }

  void checkECEnable(FelixController* flx, const std::vector<FelixTools::FelixID_t>& fids, const std::string& label, bool exclusive) {
    if (fids.empty()) return;

    logger->info("Checking EC ({}) enable registers...", label);

    if (exclusive) {
      if ( flx->getECEnableExclusive(fids) ) {
        logger->info(" All relevant EC ({}) channeles are enabled and all others are disabled!", label);
      } else {
        logger->warn(" Not all relevant EC ({}) channels are enabled or not all other EC channels are disabled!", label);
      }
    } else {
      if ( flx->getECEnable(fids) ) {
        logger->info(" All relevant EC ({}) channeles are enabled!", label);
      } else {
        logger->warn(" Not all relevant EC ({}) channels are enabled!", label);
      }
    }
  }

  void checkELinkBandWidth(FelixController* flx, const std::vector<FelixTools::FelixID_t>& fids, unsigned bandwidth, const std::string& label) {
    if (fids.empty()) return;

    logger->info("Checking e-link ({}) bandwidths...", label);
    bool allGood {true};

    for (const auto& fid : fids) {
      unsigned bw_fid = flx->getELinkWidthMbps(fid);
      if (bw_fid != bandwidth) {
        allGood = false;
        logger->warn(" FID 0x{:x} width is {} Mbps instead of {} Mbps!", fid, bw_fid, bandwidth);
      }
    }

    if (allGood) logger->info(" All e-links are {} Mbps!", bandwidth);
  }

  void checkPathEncoding(FelixController* flx, const std::vector<FelixTools::FelixID_t>& fids, unsigned encoding, const std::string& label) {
    if (fids.empty()) return;

    logger->info("Checking e-link ({}) path encoding...", label);
    bool allGood {true};

    for (const auto& fid : fids) {
      unsigned encoding_fid = flx->getPathEncodingDecoding(fid);
      if (encoding_fid != encoding) {
        allGood = false;
        logger->warn(" FID 0x{:x} path encoding is 0x{:X} instead of 0x{:x}!", fid, encoding_fid, encoding);
      }
    }

    if (allGood) logger->info(" All e-links have path encoding {}!", encoding);
  }

  void checkELinkEnable(FelixController* flx, const std::vector<FelixTools::FelixID_t>& fids, const std::string& label, bool exclusive) {
    if (fids.empty()) return;

    logger->info("Checking e-link ({}) enable registers...", label);

    if (exclusive) {
      if ( flx->getELinkEnableExclusive(fids) ) {
        logger->info(" All specified e-links ({}) are enabled and others are disabled!", label);
      } else {
        logger->warn(" Not all specified e-links ({}) are enabled or not all other e-links are disabled!", label);
      }
    } else {
      if ( flx->getELinkEnable(fids) ) {
        logger->info(" All specified e-links ({}) are enabled!", label);
      } else {
        logger->warn(" Not all specified e-links ({}) are enabled!", label);
      }
    }
  }

  void enableICs(FelixController* flx, const std::vector<FelixTools::FelixID_t>& fids, const std::string& label, bool exclusive) {
    if (fids.empty()) return;

    logger->info("Enable IC ({}) channels...", label);

    bool success = exclusive ? flx->setICEnableExclusive(fids) : flx->setICEnable(fids);
    if (success) {
      logger->info(" ...done!");
    }
  }

  void enableECs(FelixController* flx, const std::vector<FelixTools::FelixID_t>& fids, const std::string& label, bool exclusive) {
    if (fids.empty()) return;

    logger->info("Enable EC ({}) channels...", label);

    bool success = exclusive ? flx->setECEnableExclusive(fids) : flx->setECEnable(fids);
    if (success) {
      logger->info(" ...done!");
    }
  }

  void setELinkBandWidth(FelixController* flx, const std::vector<FelixTools::FelixID_t>& fids, unsigned bandwidth, const std::string& label) {
    if (fids.empty()) return;

    logger->info("Setting e-link ({}) bandwidth to {} Mbps...", label, bandwidth);
    if ( flx->setELinkWidthMbps(fids, bandwidth) ) {
      logger->info(" ...done!");
    }
  }

  void setPathEncoding(FelixController* flx, const std::vector<FelixTools::FelixID_t>& fids, unsigned encoding, const std::string& label) {
    if (fids.empty()) return;

    logger->info("Setting e-link ({}) path encoding to 0x{:x}...", label, encoding);
    if ( flx->setPathEncodingDecoding(fids, encoding) ) {
      logger->info(" ...done!");
    }
  }

  void enableELinks(FelixController* flx, const std::vector<FelixTools::FelixID_t>& fids, const std::string& label, bool exclusive) {
    if (fids.empty()) return;

    logger->info("Enable e-links ({})...", label);

    bool success = exclusive ? flx->setELinkEnableExclusive(fids) : flx->setELinkEnable(fids);
    if (success) {
      logger->info(" ...done!");
    }
  }

  json setupDefaultLoggers(bool verbose) {
    json jlog;
    jlog["pattern"] = logging::defaultLogPattern;

    jlog["log_config"][0]["name"] = "elinkConfig";
    jlog["log_config"][0]["level"] = verbose ? "debug" : "info";

    jlog["log_config"][1]["name"] = "FelixController";
    jlog["log_config"][1]["level"] = verbose ? "debug" : "info";

    jlog["log_config"][2]["name"] = "FelixTxCore";
    jlog["log_config"][2]["level"] = "info";

    jlog["log_config"][3]["name"] = "FelixRxCore";
    jlog["log_config"][3]["level"] = "info";

    if (verbose) { // add additional loggers
      jlog["log_config"][4]["name"] = "ScanHelper";
      jlog["log_config"][4]["level"] = "info";
    }

    return jlog;
  }

} // end of namespace

int main(int argc, char **argv) {
  std::string logCfg;
  std::string conCfg;
  std::vector<unsigned> rxChannels;
  std::vector<unsigned> txChannels;
  unsigned rxBandWidth {0}; // Mbps
  bool rxBandWidthProvided {false};
  bool exclusive {false};
  bool includeIC {false};
  bool includeEC {false};
  bool includeEncodingDecoding {false};
  uint8_t txEncoding {0};
  bool txEncodingProvided {false};
  uint8_t rxDecoding {0};
  bool rxDecodingProvided {false};
  bool verbose {false};

  if (argc < 3) {
    printHelp();
    return 0;
  }

  // positional arguments
  std::string cmd = argv[optind];
  std::string ctrlCfg = argv[optind+1];

  const struct option long_options[] = {
    {"help", no_argument, nullptr, 'h'},
    {"tx-encoding", required_argument, nullptr, 'T'},
    {"rx-decoding", required_argument, nullptr, 'R'},
    {"ED", no_argument, nullptr, 'C'},
    {nullptr, 0, nullptr, 0}

    };

  int opt;
  while ((opt = getopt_long(argc, argv, "hl:t:r:c:b:eIEv", long_options, nullptr)) != -1) {
    switch(opt) {
    case 'h':
      printHelp();
      return 0;
    case 'l':
      logCfg = std::string(optarg);
      break;
    case 't':
      txChannels.clear();
      optind -= 1;
      for (; optind < argc && *argv[optind] != '-'; optind += 1) {
        try { // Try parsing as number and throw if not
          txChannels.push_back( std::stoi(argv[optind]) );
        } catch (std::exception &e) {
          break;
        }
      }
      break;
    case 'r':
      rxChannels.clear();
      optind -= 1;
      for (; optind < argc && *argv[optind] != '-'; optind += 1) {
        try { // Try parsing as number and throw if not
          rxChannels.push_back( std::stoi(argv[optind]) );
        } catch (std::exception &e) {
          break;
        }
      }
      break;
    case 'c':
      conCfg = std::string(optarg);
      break;
    case 'b':
      rxBandWidth = std::stoi(optarg);
      rxBandWidthProvided = true;
      break;
    case 'e':
      exclusive = true;
      break;
    case 'I':
      includeIC = true;
      break;
    case 'E':
      includeEC = true;
      break;
    case 'C':
      includeEncodingDecoding = true;
      break;
    case 'T':
      txEncoding = std::stoi(optarg);
      txEncodingProvided = true;
      break;
    case 'R':
      rxDecoding = std::stoi(optarg);
      rxDecodingProvided = true;
      break;
    case 'v':
      verbose = true;
      break;
    default:
      spdlog::critical("Error whille parsing command line parameters!");
      return -1;
    }
  }

  // configure logger
  try {
    json jlog;
    if (logCfg.empty()) { // default
      jlog = setupDefaultLoggers(verbose);
    } else {
      jlog = ScanHelper::openJsonFile(logCfg);
    }
    logging::setupLoggers(jlog);
  } catch (std::runtime_error &e) {
    spdlog::error("Failed to load logger config: {}", e.what());
    return -1;
  }

  //////
  // Configure controller
  if (ctrlCfg.empty()) {
    logger->error("No controller configuration provided");
    return 1;
  }

  json jctrl;
  try {
    logger->debug("Loading controller config: {}", ctrlCfg);
    jctrl = ScanHelper::openJsonFile(ctrlCfg);
  } catch (std::runtime_error &e) {
    logger->critical("Failed to load controller config: {}", e.what());
    return 1;
  }

  if (jctrl["ctrlCfg"]["type"] != "FelixClient") {
    logger->critical("The controller type is not FelixClient");
    return 1;
  }
  auto hwCtrl = ScanHelper::loadController(jctrl);
  auto* flxCtrlPtr = dynamic_cast<FelixController*>(hwCtrl.get());
  if(jctrl["ctrlCfg"].contains("Card")){
    auto cardCfg = jctrl["ctrlCfg"]["Card"];
    // Command line arguments override config file parameters
    txEncoding = txEncodingProvided ? txEncoding : cardCfg["txEncoding"].get<uint8_t>();
    rxDecoding = rxDecodingProvided ? rxDecoding : cardCfg["rxDecoding"].get<uint8_t>();
    rxBandWidth = rxBandWidthProvided ? rxBandWidth : cardCfg["rxBandWidth"].get<uint16_t>();
  }

  //////
  // Parse connectivity config if provided
  if (not conCfg.empty()) {
    json jchips;
    try {
      logger->debug("Loading connectivity config: {}", conCfg);
      jchips = ScanHelper::openJsonFile(conCfg);
    } catch (std::runtime_error &e) {
      logger->critical("Failed to load connectivity config: {}", e.what());
    }

    // loop over chips
    for (unsigned i = 0; i < jchips["chips"].size(); i++) {
      logger->debug(" Chip #{}", i);
      json& chip = jchips["chips"][i];
      if (chip.contains("enable") and chip["enable"]==0) { // skip
        logger->debug("  not enabled, skipping!");
        continue;
      }
      rxChannels.push_back(chip["rx"]);
      txChannels.push_back(chip["tx"]);
    }
  }

  if (txChannels.empty() and rxChannels.empty() and not (cmd=="off" or cmd=="OFF")) {
    logger->warn("No Tx or Rx channels specified! Please provide channel numbers using the option '-t', '-r', and/or '-c'.");
    return 0;
  }

  //////
  // Remove duplicates and convert to FELIX IDs
  std::set<FelixTools::FelixID_t> fids_tx;
  for (unsigned tx : txChannels) {
    if (flxCtrlPtr->fwMode() == FelixTools::FELIX_FW_MODE::ITK_Strip) {
      // Special case: Strip LCB encoder
      auto [lcb_cfg, lcb_cmd, lcb_trkl] = FelixTools::lcbChns_from_chn(tx);
      fids_tx.insert(flxCtrlPtr->FelixTxCore::fid_from_channel(lcb_cfg));
      fids_tx.insert(flxCtrlPtr->FelixTxCore::fid_from_channel(lcb_cmd));
      fids_tx.insert(flxCtrlPtr->FelixTxCore::fid_from_channel(lcb_trkl));
    } else {
      fids_tx.insert(flxCtrlPtr->FelixTxCore::fid_from_channel(tx));
    }
  }

  std::set<FelixTools::FelixID_t> fids_rx;
  for (unsigned rx : rxChannels) {
    fids_rx.insert(flxCtrlPtr->FelixRxCore::fid_from_channel(rx));
  }

  std::vector<FelixTools::FelixID_t> vfids_tx (fids_tx.begin(), fids_tx.end());
  std::vector<FelixTools::FelixID_t> vfids_rx (fids_rx.begin(), fids_rx.end());


  if (cmd == "get" or cmd == "GET") {
    if (includeIC) {
      checkICEnable(flxCtrlPtr, vfids_tx, "Tx", exclusive);
      checkICEnable(flxCtrlPtr, vfids_rx, "Rx", exclusive);
    }

    if (includeEC) {
      checkECEnable(flxCtrlPtr, vfids_tx, "Tx", exclusive);
      checkECEnable(flxCtrlPtr, vfids_rx, "Rx", exclusive);
    }

    if (rxBandWidth > 0) {
      checkELinkBandWidth(flxCtrlPtr, vfids_rx, rxBandWidth, "Rx");
    }

    if(includeEncodingDecoding){
      checkPathEncoding(flxCtrlPtr, vfids_tx, txEncoding, "Tx");
      checkPathEncoding(flxCtrlPtr, vfids_rx, rxDecoding, "Rx");
    }

    checkELinkEnable(flxCtrlPtr, vfids_tx, "Tx", exclusive);
    checkELinkEnable(flxCtrlPtr, vfids_rx, "Rx", exclusive);
  }
  else if (cmd == "set" or cmd == "SET") {
    if (includeIC) {
      enableICs(flxCtrlPtr, vfids_tx, "Tx", exclusive);
      enableICs(flxCtrlPtr, vfids_rx, "Rx", exclusive);
    }

    if (includeEC) {
      enableECs(flxCtrlPtr, vfids_tx, "Tx", exclusive);
      enableECs(flxCtrlPtr, vfids_rx, "Rx", exclusive);
    }

    if (rxBandWidth > 0) {
      setELinkBandWidth(flxCtrlPtr, vfids_rx, rxBandWidth, "Rx");
    }

    if(includeEncodingDecoding){
      setPathEncoding(flxCtrlPtr, vfids_tx, txEncoding, "Tx");
      setPathEncoding(flxCtrlPtr, vfids_rx, rxDecoding, "Rx");
    }

    enableELinks(flxCtrlPtr, vfids_tx, "Tx", exclusive);
    enableELinks(flxCtrlPtr, vfids_rx, "Rx", exclusive);
  }
  else if (cmd == "off" or cmd == "OFF") {
    flxCtrlPtr->disableAllELinks(true);
    flxCtrlPtr->disableAllELinks(false);

    if (includeIC) {
      flxCtrlPtr->disableAllICs(true);
      flxCtrlPtr->disableAllICs(false);
    }

    if (includeEC) {
      flxCtrlPtr->disableAllECs(true);
      flxCtrlPtr->disableAllECs(false);
    }
  }
  else {
    logger->error("Unknown command {}. Possible commands are: 'get', 'set'", cmd);
  }

  return 0;
}