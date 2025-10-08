#include "LCBUtils.h"
#include "StarCLIUtils.h"
#include "logging.h"

#include <getopt.h>
#include <iomanip>
#include <iostream>

namespace {

auto logger = logging::make_log("lcbFastCommand");

std::map<std::string, LCB::FastCmdType> commandTypes = {
    {"logic-reset", LCB::LOGIC_RESET},
    {"abc-reg-reset", LCB::ABC_REG_RESET},
    {"abc-seu-reset", LCB::ABC_SEU_RESET},
    {"abc-cal-pulse", LCB::ABC_CAL_PULSE},
    {"abc-digital-pulse", LCB::ABC_DIGITAL_PULSE},
    {"abc-hit-count-reset", LCB::ABC_HIT_COUNT_RESET},
    {"abc-hit-count-start", LCB::ABC_HIT_COUNT_START},
    {"abc-hit-count-stop", LCB::ABC_HIT_COUNT_STOP},
    {"abc-slow-command-reset", LCB::ABC_SLOW_COMMAND_RESET},
    {"hcc-stop-prlp", LCB::HCC_STOP_PRLP},
    {"hcc-reg-reset", LCB::HCC_REG_RESET},
    {"hcc-seu-reset", LCB::HCC_SEU_RESET},
    {"hcc-pll-reset", LCB::HCC_PLL_RESET},
    {"hcc-start-prlp", LCB::HCC_START_PRLP}};

struct Arguments {
  std::string controllerConfigPath = "";
  LCB::FastCmdType type = LCB::NONE;
  uint32_t tx = 1;
  uint8_t delay = 0;
};

void showHelp() {
  std::cout << "Usage: lcbFastCommand <controller-config-path> <fast-command> "
               "<options>\n";
  std::cout << "  <controller-config-path> : Path to controller configuration "
               "file.\n";
  std::cout << "  <fast-command> : Fast command to send.\n";
  std::cout << "  -h, --help: Show this help.\n";
  std::cout << "  --show-fast-commands: Show available fast commands.\n";
  std::cout << "  -t, --tx <channel> : Tx channel."
            << "  (Default: " << Arguments().tx << ")\n";
  std::cout << "  -d, --delay <delay> : Delay."
            << "  (Default: " << static_cast<int>(Arguments().delay) << ")\n";
  std::cout << std::endl;
}

void showFastCommands() {
  // kind of annoying, because the map is most useful with strings as keys
  // but that means the output is sorted by the string! so we need to rebuild
  // a map with the integer as key to get sorted output.

  std::map<int, std::string> invMap;
  for (const auto &cmd : commandTypes) {
    invMap[static_cast<int>(cmd.second)] = cmd.first;
  }


  // print our stuff
  std::cout << "The fast command can be either the name or the integer code.\n";
  std::stringstream ss; // use a separate stream so we can format how we like
  for (const auto &cmd : invMap) {
    ss << " (" << std::setw(2) << std::right << cmd.first << ") "
              << std::setw(0) << std::left << cmd.second << "\n";
  }
  std::cout << ss.str();
}

Arguments processArguments(int argc, char *argv[]) {
  Arguments args;

  enum {
    SHOW_FAST_COMMANDS_ARG = 256,
  };

  const struct option long_options[] = {
      {"help", no_argument, nullptr, 'h'},
      {"show-fast-commands", no_argument, nullptr, SHOW_FAST_COMMANDS_ARG},
      {"tx", required_argument, nullptr, 't'},
      {"delay", required_argument, nullptr, 'd'},
      {nullptr, 0, nullptr, 0}};

  int c;
  while ((c = getopt_long(argc, argv, "ht:d:", long_options, nullptr)) != -1) {
    switch (c) {
    case 'h':
      showHelp();
      exit(0);
    case SHOW_FAST_COMMANDS_ARG:
      showHelp();
      showFastCommands();
      exit(0);
    case 't':
      args.tx = std::stoi(optarg);
      break;
    case 'd':
      args.delay = std::stoi(optarg);
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
    if (typeInt < 0 or typeInt > 15) {
      logger->error("Fast command type must be between 0 and 15.");
      exit(1);
    }
    args.type = static_cast<LCB::FastCmdType>(typeInt);
  } catch (std::invalid_argument &) {
    if (commandTypes.find(argv[optind + 1]) == commandTypes.end()) {
      logger->error("Unknown fast command: {}", argv[optind + 1]);
      exit(1);
    }
    args.type = commandTypes[argv[optind + 1]];
  }

  return args;
}

} // namespace

int main(int argc, char *argv[]) {
  StarCLIUtils::setLoggingDefaults("lcbFastCommand");
  auto args = processArguments(argc, argv);

  std::unique_ptr<HwController> hwCtrl =
      StarCLIUtils::createHwController(args.controllerConfigPath);
  StarCLIUtils::prepareTx(*hwCtrl, args.tx);
  StarCLIUtils::sendFastCommand(*hwCtrl, args.type, args.delay);

  return 0;
}