#include <getopt.h>
#include <memory>
#include <set>

#include "logging.h"
#include "LoggingConfig.h"
#include "ScanOpts.h"
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
    std::cout << "Options:" << std::endl;
    std::cout << " -h : Show this help." << std::endl;
    std::cout << " -t TX_CHANNELS : A list of Tx channels to be configured." << std::endl;
    std::cout << " -r RX_CHANNELS : A list of Rx channels to be configured." << std::endl;
    std::cout << " -c CHIP_CONFIG : Connectivity configuration file." << std::endl;
    std::cout << " -b BANDWIDTH : Rx bandwidth in Mbps." << std::endl;
    std::cout << " -I : Include IC channels" << std::endl;
    std::cout << " -E : Include EC channels" << std::endl;
    std::cout << " -l LOG_CONFIG : Configuration for the logger." << std::endl;
    std::cout << " -v : Verbose mode. Set logging level to 'debug'. Overwritten by '-l LOG_CONFIG' if a logging configuration is provided." << std::endl;
    std::cout << " " << std::endl;
  }

  void checkICEnable(FelixController* flx, const std::vector<FelixTools::FelixID_t>& fids, const std::string& label) {
    if (fids.empty()) return;

    logger->info("Checking IC ({}) enable registers...", label);
    if ( flx->getICEnable(fids) ) {
      logger->info(" All relevant IC ({}) channels are enabled!", label);
    } else {
      logger->warn(" Not all relevant IC ({}) channels are enabled!", label);
    }
  }

  void checkECEnable(FelixController* flx, const std::vector<FelixTools::FelixID_t>& fids, const std::string& label) {
    if (fids.empty()) return;

    logger->info("Checking EC ({}) enable registers...", label);
    if ( flx->getECEnable(fids) ) {
      logger->info(" All relevant EC ({}) channels are enabled!", label);
    } else {
      logger->warn(" Not all relevant EC ({}) channels are enabled!", label);
    }
  }

  void checkELinkBandWidth(FelixController* flx, const std::vector<FelixTools::FelixID_t>& fids, unsigned bandwidth, const std::string& label) {
    if (fids.empty()) return;

    logger->info("Checking E-link ({}) bandwidths...", label);
    bool allGood {true};

    for (const auto& fid : fids) {
      unsigned bw_fid = flx->getELinkWidthMbps(fid);
      if (bw_fid != bandwidth) {
        allGood = false;
        logger->warn(" FID 0x{:x} width is {} Mbps instead of {} Mbps!", fid, bw_fid, bandwidth);
      }
    }

    if (allGood) logger->info(" All E-links are {} Mbps!", bandwidth);
  }

  void checkELinkEnable(FelixController* flx, const std::vector<FelixTools::FelixID_t>& fids, const std::string& label) {
    if (fids.empty()) return;

    logger->info("Checking E-link ({}) enable registers...", label);
    if ( flx->getELinkEnable(fids) ) {
      logger->info(" All E-links ({}) are enabled!", label);
    } else {
      logger->warn(" Not all E-links ({}) are enabled!", label);
    }
  }

  void enableICs(FelixController* flx, const std::vector<FelixTools::FelixID_t>& fids, const std::string& label) {
    if (fids.empty()) return;

    logger->info("Enable IC ({}) channels...", label);
    if ( flx->setICEnable(fids) ) {
      logger->info(" ...done!");
    }
  }

  void enableECs(FelixController* flx, const std::vector<FelixTools::FelixID_t>& fids, const std::string& label) {
    if (fids.empty()) return;

    logger->info("Enable EC ({}) channels...", label);
    if ( flx->setECEnable(fids) ) {
      logger->info(" ...done!");
    }
  }

  void setELinkBandWidth(FelixController* flx, const std::vector<FelixTools::FelixID_t>& fids, unsigned bandwidth, const std::string& label) {
    if (fids.empty()) return;

    logger->info("Setting E-link ({}) bandwidth to {} Mbps...", label, bandwidth);
    if ( flx->setELinkWidthMbps(fids, bandwidth) ) {
      logger->info(" ...done!");
    }
  }

  void enableELinks(FelixController* flx, const std::vector<FelixTools::FelixID_t>& fids, const std::string& label) {
    if (fids.empty()) return;

    logger->info("Enable E-links ({})...", label);
    if ( flx->setELinkEnable(fids) ) {
      logger->info(" ...done!");
    }
  }

}

int main(int argc, char **argv) {
  std::string logCfg;
  std::string conCfg;
  std::vector<unsigned> rxChannels;
  std::vector<unsigned> txChannels;
  unsigned rxBandWidth {0}; // Mbps
  bool includeIC {false};
  bool includeEC {false};
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
    {nullptr, 0, nullptr, 0}
    };

  int opt;
  while ((opt = getopt_long(argc, argv, "hl:t:r:c:b:IEv", long_options, nullptr)) != -1) {
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
      break;
    case 'I':
      includeIC = true;
      break;
    case 'E':
      includeEC = true;
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
  if (logCfg.empty()) { // default
    ScanOpts options;
    json jlog;
    jlog["pattern"] = options.defaultLogPattern;
    jlog["log_config"][0]["name"] = "all";
    if (verbose) {
      jlog["log_config"][0]["level"] = "debug";
    } else {
      jlog["log_config"][0]["level"] = "info";
    }
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

  if (txChannels.empty() and rxChannels.empty()) {
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
      checkICEnable(flxCtrlPtr, vfids_tx, "Tx");
      checkICEnable(flxCtrlPtr, vfids_rx, "Rx");
    }

    if (includeEC) {
      checkECEnable(flxCtrlPtr, vfids_tx, "Tx");
      checkECEnable(flxCtrlPtr, vfids_rx, "Rx");
    }

    if (rxBandWidth > 0) {
      checkELinkBandWidth(flxCtrlPtr, vfids_rx, rxBandWidth, "Rx");
    }

    checkELinkEnable(flxCtrlPtr, vfids_tx, "Tx");
    checkELinkEnable(flxCtrlPtr, vfids_rx, "Rx");
  }
  else if (cmd == "set" or cmd == "SET") {
    if (includeIC) {
      enableICs(flxCtrlPtr, vfids_tx, "Tx");
      enableICs(flxCtrlPtr, vfids_rx, "Rx");
    }

    if (includeEC) {
      enableECs(flxCtrlPtr, vfids_tx, "Tx");
      enableECs(flxCtrlPtr, vfids_rx, "Rx");
    }

    if (rxBandWidth > 0) {
      setELinkBandWidth(flxCtrlPtr, vfids_rx, rxBandWidth, "Rx");
    }

    enableELinks(flxCtrlPtr, vfids_tx, "Tx");
    enableELinks(flxCtrlPtr, vfids_rx, "Rx");
  }
  else {
    logger->error("Unknown command {}. Possible commands are: 'get', 'set'", cmd);
  }

  return 0;
}