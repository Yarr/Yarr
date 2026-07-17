#include "FelixTools.h"
#include "LCBUtils.h"
#include "LCBFwUtils.h"
#include "StarCLIUtils.h"
#include "logging.h"

#include <getopt.h>
#include <iomanip>
#include <iostream>

namespace {

auto logger = logging::make_log("lcbFastCommand");

std::map<std::string, LCB_FELIX::ConfigReg> registerNames = {
  {"L0A_FRAME_PHASE", LCB_FELIX::L0A_FRAME_PHASE},
  {"L0A_FRAME_DELAY", LCB_FELIX::L0A_FRAME_DELAY},
  {"TTC_L0A_ENABLE", LCB_FELIX::TTC_L0A_ENABLE},
  {"TTC_BCR_DELAY", LCB_FELIX::TTC_BCR_DELAY},
  {"GATING_TTC_ENABLE", LCB_FELIX::GATING_TTC_ENABLE},
  {"GATING_BC_START", LCB_FELIX::GATING_BC_START},
  {"GATING_BC_STOP", LCB_FELIX::GATING_BC_STOP},
  {"TRICKLE_TRIGGER_PULSE", LCB_FELIX::TRICKLE_TRIGGER_PULSE},
  {"TRICKLE_TRIGGER_RUN", LCB_FELIX::TRICKLE_TRIGGER_RUN},
  {"TRICKLE_DATA_START", LCB_FELIX::TRICKLE_DATA_START},
  {"TRICKLE_DATA_END", LCB_FELIX::TRICKLE_DATA_END},
  {"TRICKLE_WRITE_ADDR", LCB_FELIX::TRICKLE_WRITE_ADDR},
  {"TRICKLE_SET_WRITE_ADDR_PULSE", LCB_FELIX::TRICKLE_SET_WRITE_ADDR_PULSE},
  {"ENCODING_ENABLE", LCB_FELIX::ENCODING_ENABLE},
  {"HCC_MASK", LCB_FELIX::HCC_MASK},
  {"ABC_MASK_0", LCB_FELIX::ABC_MASK_0},
  {"ABC_MASK_1", LCB_FELIX::ABC_MASK_1},
  {"ABC_MASK_2", LCB_FELIX::ABC_MASK_2},
  {"ABC_MASK_3", LCB_FELIX::ABC_MASK_3},
  {"ABC_MASK_4", LCB_FELIX::ABC_MASK_4},
  {"ABC_MASK_5", LCB_FELIX::ABC_MASK_5},
  {"ABC_MASK_6", LCB_FELIX::ABC_MASK_6},
  {"ABC_MASK_7", LCB_FELIX::ABC_MASK_7},
  {"ABC_MASK_8", LCB_FELIX::ABC_MASK_8},
  {"ABC_MASK_9", LCB_FELIX::ABC_MASK_9},
  {"ABC_MASK_A", LCB_FELIX::ABC_MASK_A},
  {"ABC_MASK_B", LCB_FELIX::ABC_MASK_B},
  {"ABC_MASK_C", LCB_FELIX::ABC_MASK_C},
  {"ABC_MASK_D", LCB_FELIX::ABC_MASK_D},
  {"ABC_MASK_E", LCB_FELIX::ABC_MASK_E},
  {"ABC_MASK_F", LCB_FELIX::ABC_MASK_F}
};

struct Arguments {
  std::string controllerConfigPath = "";
  LCB_FELIX::ConfigReg reg = LCB_FELIX::L0A_FRAME_PHASE;
  uint32_t tx = 0;
  uint16_t value = 0;
};

void showHelp() {
  std::cout << "Usage: starFelixCommands <controller-config-path> <register-name> <options>\n";
  std::cout << "  <controller-config-path> : Path to controller configuration "
               "file.\n";
  std::cout << "  -r <register-name> : Star FW register to set (either string or integer).\n";
  std::cout << "  -h, --help: Show this help.\n";
  std::cout << "  --show-registers: Show available registers.\n";
  std::cout << "  -t, --tx <channel> : Tx channel."
            << "  (Default: " << Arguments().tx << ")\n";
  std::cout << "  -d, --data <value> : Register value."
            << "  (Default: " << static_cast<int>(Arguments().value) << ")\n";
  std::cout << std::endl;
}

void showRegisterList() {
  // kind of annoying, because the map is most useful with strings as keys
  // but that means the output is sorted by the string! so we need to rebuild
  // a map with the integer as key to get sorted output.

  std::map<int, std::string> invMap;
  for (const auto &cmd : registerNames) {
    invMap[static_cast<int>(cmd.second)] = cmd.first;
  }

  // print our stuff
  std::cout << "Known register list:\n";
  std::stringstream ss;
  for (const auto &cmd : invMap) {
    ss << " (" << std::setw(2) << std::right << cmd.first << ") "
              << std::setw(0) << std::left << cmd.second << "\n";
  }
  std::cout << ss.str();
}

Arguments processArguments(int argc, char *argv[]) {
  Arguments args;

  enum {
    SHOW_REGISTERS_ARG = 256,
  };

  const struct option long_options[] = {
      {"help", no_argument, nullptr, 'h'},
      {"show-registers", no_argument, nullptr, SHOW_REGISTERS_ARG},
      {"tx", required_argument, nullptr, 't'},
      {"delay", required_argument, nullptr, 'd'},
      {nullptr, 0, nullptr, 0}};

  int c;
  while ((c = getopt_long(argc, argv, "ht:d:", long_options, nullptr)) != -1) {
    switch (c) {
    case 'h':
      showHelp();
      exit(0);
    case SHOW_REGISTERS_ARG:
      showRegisterList();
      exit(0);
    case 't':
      args.tx = std::stoi(optarg);
      break;
    case 'd':
      args.value = std::stoi(optarg);
      break;
    default:
      logger->error("Error while parsing command line arguments!");
      exit(1);
    }
  }

  if (optind + 2 != argc) {
    logger->error("Incorrect number of positional arguments.");
    logger->error("Expect <controller-config-path> <fast-command>");
    exit(1);
  }

  args.controllerConfigPath = argv[optind];

  try {
    int typeInt = std::stoi(argv[optind + 1]);
    if (typeInt < 0 || typeInt > 0x1e) {
      logger->error("Known Registers between 0 and 30.");
      exit(1);
    }
    args.reg = static_cast<LCB_FELIX::ConfigReg>(typeInt);
  } catch (std::invalid_argument &) {
    if (registerNames.find(argv[optind + 1]) == registerNames.end()) {
      logger->error("Unknown fast command: {}", argv[optind + 1]);
      exit(1);
    }
    args.reg = registerNames[argv[optind + 1]];
  }

  return args;
}

} // namespace

int main(int argc, char *argv[]) {
  //  StarCLIUtils::setLoggingDefaults("lcbFastCommand");
  auto args = processArguments(argc, argv);

  std::unique_ptr<HwController> hwCtrl =
      StarCLIUtils::createHwController(args.controllerConfigPath);

  auto elink_num = args.tx;
  auto [lcb_cfg, lcb_cmd, lcb_trkl] = FelixTools::lcbChns_from_chn(elink_num);

  // Enable configuration elink
  StarCLIUtils::prepareTx(*hwCtrl, lcb_cfg);

  auto data_word = LCB_FELIX::config_command(args.reg, args.value);

  hwCtrl->writeFifo(data_word);
  hwCtrl->releaseFifo();

  return 0;
}
