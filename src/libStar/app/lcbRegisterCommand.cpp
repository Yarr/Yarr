#include "RawData.h"
#include "ScanHelper.h"
#include "StarCLIUtils.h"
#include "StarChipPacket.h"
#include "logging.h"

#include <getopt.h>
#include <iostream>

namespace {

auto logger = logging::make_log("lcbRegisterCommand");

struct Arguments {
  std::string controllerConfigPath = "";
  bool isRead = true;
  bool isHcc = true;
  uint32_t tx = 1;
  uint32_t rx = 0;
  int hccId = 0xf;
  int abcId = 0xf;
  int address = 0;
  bool addressIsSet = false;
  uint32_t value = 0;
  bool valueIsSet = false;
  bool sendOnly = false;
  uint32_t timeout = 1000; // milliseconds
};

void showHelp() {
  std::cout << "Usage: lcbRegisterCommand <controller-config-path> <action> "
               "<chip-type> <options>\n";
  std::cout << "  <controller-config-path> : Path to controller configuration "
               "file.\n";
  std::cout << "  <action> : read | write\n";
  std::cout << "  <chip-type> : hcc | abc\n";
  std::cout << "  -h, --help: Show this help.\n";
  std::cout << "  -t, --tx <channel> : Tx channel."
            << "  (Default: " << Arguments().tx << ")\n";
  std::cout << "  -r, --rx <channel> : Rx channel."
            << "  (Default: " << Arguments().rx << ")\n";
  std::cout << "  -i, --hcc-id <id> : HCC id."
            << "  (Default: " << Arguments().hccId << ")\n";
  std::cout << "  -a, --abc-id <id> : ABC id."
            << "  (Default: " << Arguments().abcId << ")\n";
  std::cout << "  -d, --address <address> : Register address."
            << "  (Default: " << Arguments().address << ")\n";
  std::cout << "  -v, --value <value> : Value to write (only for write)."
            << "  (Default: " << Arguments().value << ")\n";
  std::cout
      << "  -s, --send-only : Do not actually read data back (only for read)."
      << "  (Default: " << (Arguments().isRead ? "false" : "true") << ")\n";
  std::cout << "  -m, --timeout <ms> : Timeout in milliseconds to wait for the "
               "expected data (only for read)."
            << "  (Default: " << Arguments().timeout << ")\n";
  std::cout << std::endl;
}

Arguments processArguments(int argc, char *argv[]) {
  Arguments args;

  const struct option long_options[] = {
      {"help", no_argument, nullptr, 'h'},
      {"tx", required_argument, nullptr, 't'},
      {"rx", required_argument, nullptr, 'r'},
      {"hcc-id", required_argument, nullptr, 'i'},
      {"abc-id", required_argument, nullptr, 'a'},
      {"address", required_argument, nullptr, 'd'},
      {"value", required_argument, nullptr, 'v'},
      {"send-only", no_argument, nullptr, 's'},
      {"timeout", required_argument, nullptr, 'm'},
      {nullptr, 0, nullptr, 0}};

  int c;
  while ((c = getopt_long(argc, argv, "ht:r:i:a:d:v:sm:", long_options,
                          nullptr)) != -1) {
    switch (c) {
    case 'h':
      showHelp();
      exit(0);
    case 't':
      args.tx = std::stoi(optarg);
      break;
    case 'r':
      args.rx = std::stoi(optarg);
      break;
    case 'i':
      args.hccId = std::stoi(optarg);
      if (args.hccId < 0 or args.hccId > 15) {
        logger->error("HCC id must be between 0 and 15.");
        exit(1);
      }
      break;
    case 'a':
      args.abcId = std::stoi(optarg);
      if (args.abcId < 0 or args.abcId > 15) {
        logger->error("ABC id must be between 0 and 15.");
        exit(1);
      }
      break;
    case 'd':
      args.address = std::stoi(optarg);
      if (args.address < 0 or args.address > 255) {
        logger->error("Register address must be between 0 and 255.");
        exit(1);
      }
      args.addressIsSet = true;
      break;
    case 'v':
      args.value = std::stoul(optarg, nullptr, 0);
      args.valueIsSet = true;
      break;
    case 's':
      args.sendOnly = true;
      break;
    case 'm':
      args.timeout = std::stoul(optarg);
      break;
    default:
      logger->error("Error while parsing command line arguments!");
      exit(1);
    }
  }

  if (!args.addressIsSet) {
    logger->error("Register address must be specified.");
    exit(1);
  }

  if (optind + 3 != argc) {
    logger->error("Incorrect number of positional arguments.");
    logger->error("Expect <controller-config-path> <action> <chip-type>");
    exit(1);
  }

  args.controllerConfigPath = argv[optind];

  std::string action = argv[optind + 1];
  if (action == "read") {
    args.isRead = true;
  } else if (action == "write") {
    args.isRead = false;
  } else {
    logger->error("Unknown action: {}", action);
    exit(1);
  }

  if (!args.isRead && !args.valueIsSet) {
    logger->error("Register value must be specified for write action.");
    exit(1);
  }

  std::string chipType = argv[optind + 2];
  if (chipType == "hcc") {
    args.isHcc = true;
  } else if (chipType == "abc") {
    args.isHcc = false;
  } else {
    logger->error("Unknown chip type: {}", chipType);
    exit(1);
  }

  return args;
}

} // namespace

int main(int argc, char *argv[]) {
  StarCLIUtils::setLoggingDefaults("lcbRegisterCommand");
  auto args = processArguments(argc, argv);

  std::unique_ptr<HwController> hwCtrl =
      StarCLIUtils::createHwController(args.controllerConfigPath);
  StarCLIUtils::prepareTx(*hwCtrl, args.tx);

  if (!args.isRead || args.sendOnly) {
    StarCLIUtils::sendRegisterCommand(
        *hwCtrl, args.hccId, args.abcId, args.address, args.isRead,
        static_cast<uint32_t>(args.value), args.isHcc);
  } else {
    StarCLIUtils::prepareRx(*hwCtrl, args.rx);

    StarCLIUtils::sendRegisterCommand(
        *hwCtrl, args.hccId, args.abcId, args.address, args.isRead,
        static_cast<uint32_t>(args.value), args.isHcc);

    PacketType expectedPacketType = args.isHcc ? TYP_HCC_RR : TYP_ABC_RR;
    auto data = StarCLIUtils::readData(
        *hwCtrl,
        [&](RawData &d) {
          return StarCLIUtils::isPacketType(d, expectedPacketType) &&
                 StarCLIUtils::isFromChannel(d, args.rx);
        },
        args.timeout);

    for (const auto &d : data) {
      StarChipPacket packet;
      if (StarCLIUtils::packetFromRawData(packet, *d) != 0) {
        // this is a really weird situation. we already did this in readData
        // so why would it fail now? should never get here!
        logger->error("Failed to parse the received packet");
        return 1;
      }

      std::stringstream packetInfoStream;
      packet.print_more(packetInfoStream);
      std::string packetInfo = packetInfoStream.str();
      packetInfo.erase(packetInfo.size() - 1); // remove newline
      logger->info(packetInfo);
    }

    return 0;
  }
}