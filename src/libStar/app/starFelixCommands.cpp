#include "FelixTools.h"
#include "LCBUtils.h"
#include "LCBFwUtils.h"
#include "StarCLIUtils.h"
#include "logging.h"
#include "LoggingConfig.h"

#include <getopt.h>
#include <iomanip>
#include <iostream>

namespace {

auto logger = logging::make_log("starFelixCommands");

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

std::map<int, std::string> invertedRegistersMap()
{
  std::map<int, std::string> invMap;
  for (const auto &cmd : registerNames) {
    invMap[static_cast<int>(cmd.second)] = cmd.first;
  }
  return invMap;
}

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
  auto invMap = invertedRegistersMap();

  // print our stuff
  std::cout << "Known register list:\n";
  std::stringstream ss;
  for (const auto &cmd : invMap) {
    ss << " (" << std::setw(2) << std::right << cmd.first << ") "
              << std::setw(0) << std::left << cmd.second << "\n";
  }
  std::cout << ss.str();
}

// Parse register argument as either int or string
LCB_FELIX::ConfigReg parseRegisterName(const char* reg_string) {
  try {
    // Allow specifying as hex or int
    unsigned regInt = std::stoul(optarg, nullptr, 0);
    if (regInt < 0 || regInt > 0x1e) {
      logger->error("Known registers between 0 and 30.");
      showRegisterList();
      exit(1);
    }
    return static_cast<LCB_FELIX::ConfigReg>(regInt);
  } catch (std::invalid_argument &) {
    if (registerNames.find(reg_string) == registerNames.end()) {
      logger->error("Unknown register: {}", reg_string);
      showRegisterList();
      exit(1);
    }
    return registerNames[reg_string];
  }
}

Arguments processArguments(int argc, char *argv[]) {
  Arguments args;

  enum {
    SHOW_REGISTERS_ARG = 256,
  };

  const struct option long_options[] = {
      {"help", no_argument, nullptr, 'h'},
      {"show-registers", no_argument, nullptr, SHOW_REGISTERS_ARG},
      {"register", required_argument, nullptr, 'r'},
      {"tx", required_argument, nullptr, 't'},
      {"delay", required_argument, nullptr, 'd'},
      {nullptr, 0, nullptr, 0}};

  int c;
  while ((c = getopt_long(argc, argv, "hd:r:t:R:", long_options, nullptr)) != -1) {
    switch (c) {
    case 'h':
      showHelp();
      exit(0);
    case SHOW_REGISTERS_ARG:
      showRegisterList();
      exit(0);
    case 'd':
      args.value = std::stoul(optarg, nullptr, 0);
      break;
    case 'r':
      args.reg = parseRegisterName(optarg);
      break;
    case 't':
      args.tx = std::stoi(optarg);
      break;
    case 'R':
      args.controllerConfigPath = optarg;
      break;
    default:
      logger->error("Error while parsing command line arguments!");
      showHelp();
      exit(1);
    }
  }

  if (optind != argc) {
    logger->error("Not expecting any positional arguments {}!", argc-optind);
    showHelp();
    exit(1);
  }

  return args;
}

} // namespace

int main(int argc, char *argv[]) {
  logging::setupLoggers(logging::defaultConfig());

  auto args = processArguments(argc, argv);

  auto elink_num = args.tx;
  auto [lcb_cfg, lcb_cmd, lcb_trkl] = FelixTools::lcbChns_from_chn(elink_num);

  logger->trace("Fetch links for {} -> {} {} {}",
                elink_num, lcb_cfg, lcb_cmd, lcb_trkl);

  logger->info("Write register {} ({}) with value {} on tx {}",
               invertedRegistersMap()[args.reg], static_cast<int>(args.reg), args.value, lcb_cmd);

  std::unique_ptr<HwController> hwCtrl =
      StarCLIUtils::createHwController(args.controllerConfigPath);

  // Enable configuration elink
  StarCLIUtils::prepareTx(*hwCtrl, lcb_cfg);

  auto data_word = LCB_FELIX::config_command(args.reg, args.value);

  hwCtrl->writeFifo(data_word);
  hwCtrl->releaseFifo();

  return 0;
}
