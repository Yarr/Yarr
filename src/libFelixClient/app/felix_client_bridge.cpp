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
#include "FelixTools.h"

#include "spdlog/fmt/bundled/ranges.h"

#include "storage.hpp"

#include "felix-server/Receiver.hpp"
#include "felix-server/FelixServer.hpp"

namespace {
auto logger = logging::make_log("felix_client_bridge");

volatile sig_atomic_t my_signalled_flag = 0;
}

/// Our application options, common between felix receiver and publisher
struct CommonSettings {
  std::string ip{"127.0.0.1"};
  // 0 means let server pick
  uint16_t port{0};
  std::size_t buffer_size{1024};
  std::string bus_path{"bridge_bus"};
  std::string bus_group{"bridge_group"};
  std::string bus_filename{"bridge_file"};
};

struct AppSettings {
  /// Common between publish and receive
  CommonSettings common;

  /// Filename for controller to bridge to
  std::string controller_name = "configs/controller/emuCfg.json";

  /// Log config to load
  std::string logCfgPath;
};

template<typename Publish>
void send_packet(std::span<uint32_t> data, uint32_t addr, Publish &publisher) {
  logger->debug("Send RxCore->Publish: (buf adr words) {:08x} {} {}", data[0], addr, data.size());

  for (uint32_t &d: data) {
    logger->trace(" 0x{:08x}", d);
  }

  // Map to fid
  uint64_t fid_tag = addr;
  fid_tag |= 1ULL<<60;

  // Want data in uint8_t, from RxCore uint32_t
  std::vector<uint8_t> outdata(data.size() * 4);

  for(size_t i=0; i<data.size(); i++) {
    uint32_t val = data[i];
    for (size_t b=0; b<4; b++) {
      outdata[i*4 + b] = (val >> (b*8)) & 0xff;
    }
  }

  bool retry_flag = true;
  auto status = publisher.publish(fid_tag, outdata, retry_flag);
  if(status != netio3::NetioPublisherStatus::OK) {
    logger->warn("send_packet: publish response is not OK {}", status);
  }
}

template<typename Publish>
void send_packets_from_rx_core_to_publisher(RxCore& rx_core, Publish &publisher)
{
  std::vector<RawDataPtr> dataVec = rx_core.readData();
  for (auto data : dataVec) {
    send_packet({data->getBuf(), data->getSize()}, data->getAdr(), publisher);
  }
}

void printHelp()
{
  std::cout << "Use felix-server to set up a server running a YARR controller\n";
  std::cout << "Options:\n";
  std::cout << "\t-f FILE\tConfiguration of Yarr controller to use (defaults to emu)\n";
  std::cout << "\t-l LOGCONFIG\tLogger configuration file\n";
  std::cout << "\t-H HOST Specify Host to connect to (not implemented)\n";
  std::cout << "\t-h\tReport this usage info\n";
  std::cout << "\n";
}

AppSettings parseArgs(int argc, char** argv)
{
  // Constructor sets up various defaults;
  AppSettings settings;

  char opt;
  const struct option long_options[] = {
    {"help", no_argument, nullptr, 'h'},
    {nullptr, 0, nullptr, 0}
    };
  while ((opt = getopt_long(argc, argv, "H:f:l:h", long_options, nullptr)) != -1) {
    switch (opt) {
    case 'H':
      settings.common.ip = optarg;
      break;
    case 'f':
      settings.controller_name = optarg;
      break;
    case 'l':
      settings.logCfgPath = std::string(optarg);
      break;
    case 'h':
      printHelp();
      exit(0);
    default:
      std::cout << "Bad parameter flag '" << opt << "'\n";
      printHelp();
      exit(-1);
    }
  }

  return settings;
}

int main(int argc, char** argv)
{
  // Defaults set above
  AppSettings settings = parseArgs(argc, argv);

  spdlog::info("Configuring logger ...");
  try {
    json jlog;

    if (settings.logCfgPath.empty()) {
      // default setting
      jlog = logging::defaultConfig();
    } else {
      jlog = ScanHelper::openJsonFile(settings.logCfgPath);
    }
    logging::setupLoggers(jlog);
  } catch (std::runtime_error &e) {
    spdlog::error("Failed to load logger config: {}", e.what());
    return -1;
  }

  if(optind != argc) {
    logger->error("Too many arguments {} {}", optind, argc);
    printHelp();
    return -1;
  }

  std::ifstream ctrlCfgFile(settings.controller_name);
  if(!ctrlCfgFile) {
    logger->error("Could not open config file: {}", settings.controller_name);
    return -1;
  }

  logger->debug("Reading from controller: {}", settings.controller_name);

  json ctrlCfg;
  try {
    ctrlCfg = json::parse(ctrlCfgFile);
  } catch (json::parse_error& e) {
    logger->error("Could not parse config: {}", e.what());
    return -1;
  }

  logger->info("Setup controller");
  std::string type = ctrlCfg["ctrlCfg"]["type"];
  std::unique_ptr<HwController> hwCtrl = StdDict::getHwController(type);

  hwCtrl->loadConfig(ctrlCfg["ctrlCfg"]["cfg"]);

  felix_server::FelixServer server{felix_server::EventLoopType::epoll};

  std::vector<uint64_t> recv_tags{};
  recv_tags.push_back(0x1000000000008000ULL);

  logger->info("Create buffered publisher");

  std::vector<uint64_t> pub_tags{};
  pub_tags.push_back(0x1000000000000000ULL);

  // Don't need libfabric
  auto network_type = netio3::NetworkType::ASYNCMSG;

  felix_server::BufferedPublisher::Settings pub_settings;
  pub_settings.ip = settings.common.ip;
  pub_settings.port = settings.common.port;
  pub_settings.network_type = network_type;
  pub_settings.buffer_size = settings.common.buffer_size;
  pub_settings.bus_path = settings.common.bus_path;
  pub_settings.bus_group = settings.common.bus_group;
  pub_settings.bus_filename = settings.common.bus_filename + "_PUB";

  // Not common
  pub_settings.buffer_count = 10;
  pub_settings.buffer_timeout = 100;
  
  pub_settings.on_sub = [](std::uint64_t tag, const netio3::EndPointAddress& address) {
    logger->info("Received subscription to tag {:x} {} from {}:{}", tag, FelixTools::print_fid(tag), address.address(), address.port());
  };
  pub_settings.on_unsub = [](std::uint64_t tag, const netio3::EndPointAddress& address) {
    logger->info("Received unsubscription from tag {:x} {} from {}:{}", tag, FelixTools::print_fid(tag), address.address(), address.port());
  };

  for(auto &t: pub_tags) {
    logger->info("pub_tag: 0x{:016x} {}", t, FelixTools::print_fid(t));
  }

  auto publisher{server.create_buffered_publisher(pub_settings, pub_tags)};

  logger->info("Create receiver");

  felix_server::Receiver::Settings recv_settings;
  recv_settings.ip = settings.common.ip;
  recv_settings.port = settings.common.port;
  recv_settings.network_type = network_type;
  recv_settings.buffer_size = settings.common.buffer_size;
  recv_settings.bus_path = settings.common.bus_path;
  recv_settings.bus_group = settings.common.bus_group;
  recv_settings.bus_filename = settings.common.bus_filename + "_RECV";;

  // Not common
  recv_settings.buffer_count = 10;
  recv_settings.buffered = true;

  // vs on_buffer is for complete buffer
  recv_settings.on_msg = [](std::uint64_t tag, std::span<const std::uint8_t> data, std::uint8_t status) {
    logger->info("Received message from tag {:#x} {}, status {}, size {}",
                 tag, FelixTools::print_fid(tag), status, data.size());

    auto to_hex = [] (auto &d) { return std::format("{:02x}", d); };
    logger->debug(" Message: {}", data | std::views::transform(to_hex));
  };

  for(auto &t: recv_tags) {
    logger->info("recv_tag: 0x{:016x} {}", t, FelixTools::print_fid(t));
  }

  auto message_receiver = server.create_receiver(recv_settings, recv_tags);

  std::jthread publish_thread([&](std::stop_token stoken) {
    while(!stoken.stop_requested()) {
      send_packets_from_rx_core_to_publisher(*hwCtrl, publisher);
    }
    logger->info("Shutdown RxCore publisher thread");
  });
  
  // While server is alive we keep running
  logger->info("Wait for user to finish (Ctrl-C or SIGUSR1 (kill -10))");

  my_signalled_flag = 0;

  signal(SIGINT, [](int signum){my_signalled_flag = signum;});
  signal(SIGUSR1, [](int signum){my_signalled_flag = signum;});

  while (true) {
    if(my_signalled_flag != 0) {
      logger->info("Finish main loop due to signal {}", my_signalled_flag);
      break;
    }
  }

  publish_thread.get_stop_source().request_stop();
  publish_thread.join();

  logger->info("Main loop complete {}", my_signalled_flag);

  return 0;
}
