#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>

#include <getopt.h>

#include "AllHwControllers.h"
#include "RxCore.h"
#include "TxCore.h"
#include "logging.h"
#include "LoggingConfig.h"
#include "ScanHelper.h"

#include "storage.hpp"

#include "felix-server/Receiver.hpp"
#include "felix-server/FelixServer.hpp"

namespace {
auto logger = logging::make_log("felix_client_bridge");
}

void help()
{
  std::cout << "Use felix-server to set up a server running a YARR controller\n";
  std::cout << "Options:\n";
  std::cout << "\t-f FILE\tConfiguration of Yarr controller to use (defaults to emu)\n";
  std::cout << "\t-l LOGCONFIG\tLogger configuration file\n";
  std::cout << "\t-h\tReport this usage info\n";
  std::cout << "\n";
}

int main(int argc, char** argv)
{
  std::string fname = "configs/controller/emuCfg.json";

  // logger config file
  std::string logCfgPath = "";

  char opt;
  const struct option long_options[] = {
    {"help", no_argument, nullptr, 'h'},
    {nullptr, 0, nullptr, 0}
    };
  while ((opt = getopt_long(argc, argv, "f:l:h", long_options, nullptr)) != -1) {
    switch (opt) {
    case 'f':
      fname = optarg;
      break;
    case 'l':
      logCfgPath = std::string(optarg);
      break;
    case 'h':
      help();
      return 0;
    default:
      std::cout << "Bad parameter flag '" << opt << "'\n";
      help();
      return -1;
    }
  }

  spdlog::info("Configuring logger ...");
  try {
    json jlog;

    if (logCfgPath.empty()) {
      // default setting
      jlog = logging::defaultConfig();
    } else {
      jlog = ScanHelper::openJsonFile(logCfgPath);
    }
    logging::setupLoggers(jlog);
  } catch (std::runtime_error &e) {
    spdlog::error("Failed to load logger config: {}", e.what());
    return -1;
  }

  if(optind != argc) {
    logger->error("Too many arguments {} {}", optind, argc);
    help();
    return -1;
  }

  std::ifstream ctrlCfgFile(fname);
  if(!ctrlCfgFile) {
    logger->error("Could not open config file: {}", fname);
    return -1;
  }

  json ctrlCfg;
  try {
    ctrlCfg = json::parse(ctrlCfgFile);
  } catch (json::parse_error& e) {
    logger->error("Could not parse config: {}", e.what());
    return -1;
  }

  std::string type = ctrlCfg["ctrlCfg"]["type"];
  std::unique_ptr<HwController> hwCtrl = StdDict::getHwController(type);

  hwCtrl->loadConfig(ctrlCfg["ctrlCfg"]["cfg"]);

  felix_server::FelixServer server{felix_server::EventLoopType::epoll};

  std::vector<uint64_t> recv_tags{};
  felix_server::Receiver::Settings recv_settings;
  auto receiver{server.create_receiver(recv_settings, recv_tags)};

  std::vector<uint64_t> pub_tags{};
  felix_server::BufferedPublisher::Settings pub_settings;
  auto publisher{server.create_buffered_publisher(pub_settings, pub_tags)};

  return 0;
}
