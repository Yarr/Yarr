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
    std::cout << " check : Check if all e-links specified in options are enabled." << std::endl;
    std::cout << " enable : Enable all e-links specified in options." << std::endl;
    std::cout << "Options:" << std::endl;
    std::cout << " -h : Show this help." << std::endl;
    std::cout << " -t TX_CHANNELS : A list of Tx channels to be configured." << std::endl;
    std::cout << " -r RX_CHANNELS : A list of Rx channels to be configured." << std::endl;
    std::cout << " -c CHIP_CONFIG : Connectivity configuration file." << std::endl;
    std::cout << " -I : Include IC channels" << std::endl;
    std::cout << " -E : Include EC channels" << std::endl;
    std::cout << " -l LOG_CONFIG : Configuration for the logger." << std::endl;
    std::cout << " -v : Verbose mode. Set logging level to 'debug'. Overwritten by '-l LOG_CONFIG' if a logging configuration is provided." << std::endl;
    std::cout << " " << std::endl;
  }
}

int main(int argc, char **argv) {
  std::string logCfg;
  std::string conCfg;
  std::vector<unsigned> rxChannels;
  std::vector<unsigned> txChannels;
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
  while ((opt = getopt_long(argc, argv, "hl:t:r:c:IEv", long_options, nullptr)) != -1) {
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
  // Remove duplicates
  std::set<unsigned> uniqueTxChannels;
  std::set<unsigned> uniqueTxLinks;
  for (unsigned tx : txChannels) {
    uniqueTxChannels.insert(tx);
    uniqueTxLinks.insert(FelixTools::link_from_chn(tx));
    // Special case for Strip LCB encoder
    if (flxCtrlPtr->fwMode() == FelixTools::FELIX_FW_MODE::ITK_Strip) {
      auto [lcb_cfg, lcb_cmd, lcb_trkl] = FelixTools::lcbChns_from_chn(tx);
      uniqueTxChannels.insert(lcb_cfg);
      uniqueTxChannels.insert(lcb_cmd);
      uniqueTxChannels.insert(lcb_trkl);
    }
  }

  std::set<unsigned> uniqueRxChannels;
  std::set<unsigned> uniqueRxLinks;
  for (unsigned rx : rxChannels) {
    uniqueRxChannels.insert(rx);
    uniqueRxLinks.insert(FelixTools::link_from_chn(rx));
  }

  txChannels.assign(uniqueTxChannels.begin(), uniqueTxChannels.end());
  rxChannels.assign(uniqueRxChannels.begin(), uniqueRxChannels.end());

  //////
  auto checkELinks = [&](const std::vector<unsigned>& chns, bool toflx) {
    if (chns.empty()) return;

    if ( flxCtrlPtr->getELinkEnablesAll(chns, toflx) ) {
      logger->info("All required e-links (toflx={}) are enabled!", toflx);
    } else {
      // FelixController::getELinkEnablesAll should print warnings on which ones are off
      logger->info("Some e-links (toflx={}) are not enabled!", toflx);
    }
  };

  auto checkICECs = [&](const std::set<unsigned>& links, bool toflx) {
    if (links.empty()) return;

    bool allICsEnabled {true};
    bool allECsEnabled {true};

    for (const auto& l : links) {
      if (includeIC) {
        if ( not flxCtrlPtr->getICEnable(l, toflx) ) {
          logger->warn("Link {} IC channel (toflx={}) is not enabled!", l, toflx);
          allICsEnabled = false;
        }
      }

      if (includeEC) {
        if ( not flxCtrlPtr->getECEnable(l, toflx) ) {
          logger->warn("Link {} EC channel (toflx={}) is not enabled!", l, toflx);
          allECsEnabled = false;
        }
      }
    }

    if (includeIC and allICsEnabled)
      logger->info("All required IC channels (toflx={}) are enabled!", toflx);

    if (includeEC and allECsEnabled)
      logger->info("All required EC channels (toflx={}) are enabled!", toflx);
  };

  auto enableELinks = [&](const std::vector<unsigned>& chns, bool toflx) {
    if (chns.empty()) return;

    if ( flxCtrlPtr->setELinkEnables(chns, toflx) ) {
      logger->info("Enabled all required e-links (toflx={}) successfully!", toflx);
    } else {
      logger->info("Failed to enable all required e-links (toflx={})", toflx);
    }
  };

  auto enableICECs = [&](const std::set<unsigned>& links, bool toflx) {
    if (links.empty()) return;

    bool allICsEnabled {true};
    bool allECsEnabled {true};

    for (const auto& l : links) {
      if (includeIC) {
        if ( not flxCtrlPtr->setICEnable(l, toflx) ) {
          logger->warn("Failed to enable link {} IC channel (toflx={})!", l, toflx);
          allICsEnabled = false;
        }
      }

      if (includeEC) {
        if ( not flxCtrlPtr->setECEnable(l, toflx) ) {
          logger->warn("Failed to enable link {} EC channel (toflx={})!", l, toflx);
          allECsEnabled = false;
        }
      }
    }

    if (includeIC and allICsEnabled)
      logger->info("Enabled all required IC channels (toflx={}) successfully!", toflx);

    if (includeEC and allECsEnabled)
      logger->info("Enabled all required EC channels (toflx={}) successfully!", toflx);
  };

  if (cmd == "check" or cmd == "CHECK") {
    checkELinks(txChannels, true);
    checkELinks(rxChannels, false);
    // IC and EC elinks
    checkICECs(uniqueTxLinks, true);
    checkICECs(uniqueRxLinks, false);

  } else if (cmd == "enable" or cmd == "ENABLE") {
    enableELinks(txChannels, true);
    enableELinks(rxChannels, false);
    // IC and EC elinks
    enableICECs(uniqueTxLinks, true);
    enableICECs(uniqueRxLinks, false);
//  } else if (cmd == "disable_all") {
  } else {
    logger->error("Unknown command {}. Possible commands are: 'check', 'enable', ", cmd);
  }

  return 0;
}