#include "ScanHelper.h"
#include "StarCLIUtils.h"
#include "logging.h"

#include <bitset>
#include <getopt.h>
#include <iostream>

namespace {

auto logger = logging::make_log("lcbL0a");

struct Arguments {
  std::string controllerConfigPath = "";
  uint32_t tx = 1;
  bool bcr = false;
  unsigned mask = 0x1;
  unsigned tag = 0x55;
};

void showHelp() {
  std::cout << "Usage: lcbL0a <controller-config-path> <options>\n";
  std::cout << "  <controller-config-path> : Path to controller configuration "
               "file.\n";
  std::cout << "  -h, --help: Show this help.\n";
  std::cout << "  -t, --tx <channel> : Tx channel."
            << "  (Default: " << Arguments().tx << ")\n";
  std::cout << "  -m, --mask <mask> : 4-bit binary L0a mask."
            << "  (Default: " << std::bitset<4>(Arguments().mask) << ")\n";
  std::cout << "  -g, --tag <tag> : 7-bit L0a tag."
            << "  (Default: " << Arguments().tag << ")\n";
  std::cout << "  -b, --bcr : If used, sets BCR bit.\n";
  std::cout << std::endl;
}

Arguments processArguments(int argc, char *argv[]) {
  Arguments args;

  const struct option long_options[] = {
      {"help", no_argument, nullptr, 'h'},
      {"tx", required_argument, nullptr, 't'},
      {"mask", required_argument, nullptr, 'm'},
      {"tag", required_argument, nullptr, 'g'},
      {"bcr", no_argument, nullptr, 'b'},
      {nullptr, 0, nullptr, 0}};

  int c;
  while ((c = getopt_long(argc, argv, "ht:m:g:b", long_options, nullptr)) != -1) {
    switch (c) {
    case 'h':
      showHelp();
      exit(0);
    case 't':
      args.tx = std::stoi(optarg);
      break;
    case 'm':
      args.mask = std::bitset<4>(optarg).to_ulong();
      break;
    case 'g':
      args.tag = std::stoi(optarg);
      break;
    case 'b':
      args.bcr = true;
      break;
    default:
      logger->error("Error while parsing command line arguments!");
      exit(1);
    }
  }

  if (optind + 1 != argc) {
    logger->error("Incorrect number of positional arguments.");
    logger->error("Expect <controller-config-path>");
    exit(1);
  }

  args.controllerConfigPath = argv[optind];

  return args;
}

} // namespace

int main(int argc, char *argv[]) {
  StarCLIUtils::setLoggingDefaults("lcbL0a");
  auto args = processArguments(argc, argv);

  std::unique_ptr<HwController> hwCtrl =
      StarCLIUtils::createHwController(args.controllerConfigPath);
  StarCLIUtils::prepareTx(*hwCtrl, args.tx);
  StarCLIUtils::sendL0a(*hwCtrl, args.mask, args.tag, args.bcr);

  return 0;
}