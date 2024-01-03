#include <fstream>
#include <iomanip>
#include <sstream>

#include "AllHwControllers.h"
#include "RxCore.h"
#include "TxCore.h"
#include "logging.h"
#include "LoggingConfig.h"
#include "ScanHelper.h"

#include "storage.hpp"

#include "felixbase/client.hpp"
#include "netio.hpp"

auto logger = logging::make_log("netio_bridge");

enum socket_type {
  SOCKET_HT,
  SOCKET_LL
};

struct {
  bool zerocopy = false;
  int verbosity = 0;
} app_settings;

struct input_reader {
  RxCore& rx_core;
  felix::base::FromFELIXHeader m_header;
  std::vector<uint8_t> m_fifo;

public:
  input_reader(RxCore& core)
    : rx_core(core)
  {}

  void next_message(netio::message& msg, netio::tag& tag)
  {
    std::vector<RawDataPtr> dataVec = rx_core.readData();
    for (auto data : dataVec) {
        logger->debug("Received data from RxCore: (buf adr words) {} {} {}", (void*)(data->getBuf()), data->getAdr(), data->getSize());
        for (int iw = 0; iw < data->getSize(); iw++) {
            logger->debug(" 0x{:08x}", data->get(iw));
        }

        // Based on the reverse of a combination of
        //   NetioRxCore::readData
        //   NetioHandler::addChannel
        std::array<uint8_t*, 2> data_array;
        std::array<size_t, 2> size_array;

        // Add felix::base::FromFELIXHeader
        int channel = data->getAdr();
        m_header.length = m_fifo.size() + sizeof(m_header);
        m_header.status = 0;
        m_header.elinkid = channel%64;
        m_header.gbtid = channel/64;

        tag = m_header.elinkid;

        data_array[0] = (uint8_t*)&m_header;
        size_array[0] = sizeof(m_header);

        // Add data
        m_fifo.clear();
        for(size_t i=0; i<data->getSize(); i++) {
            uint32_t val = data->get(i);
            for (size_t b=0; b<4; b++) {
                m_fifo.push_back((val >> (b*8)) & 0xff);
            }
        }

        data_array[1] = m_fifo.data();
        size_array[1] = m_fifo.size();

        // netio::message, array of data arrays, array of sizes and size of list
        msg = netio::message(data_array.data(), size_array.data(), 2);
    }
  }

  bool data_available()
  {
    return std::cin.good();
  }
};

struct output_writer {
private:
  TxCore& tx_core;

public:
  output_writer(TxCore& core)
    : tx_core(core)
  {
  }

  void process_message(netio::endpoint& ep, netio::message& msg)
  {
    auto msg_data = msg.data_copy();

    size_t ibyte = 0;
    while (ibyte < msg_data.size()) {
      felix::base::ToFELIXHeader &header = *(felix::base::ToFELIXHeader*)&msg_data[ibyte];

      logger->debug("Decode FELIX header from NetIO");
      logger->debug(" length: {}", header.length);
      logger->debug(" reserved: {}", header.reserved);
      logger->debug(" elinkid: {}", header.elinkid);
      logger->debug(" header size: {}", sizeof(header));

      // Enable the tx channel according to header.elinkid
      tx_core.setCmdEnable(header.elinkid);

      ibyte += sizeof(header);

      // Read data
      logger->trace(" words:");
      for (size_t offset = 0; offset < header.length; offset +=4) {
        uint32_t word = *(uint32_t*)&msg_data[offset + ibyte];
        // convert byte array into unsigned int
        // The above line flips the order of bytes in a little-endian system
        // e.g. {0x12, 0x34, 0x56, 0x78} becomes 0x78563412
        // but the msg_data is in big-endian order i.e. {MSB, ..., LSB}
        // for now:
        word = ((word & 0xff000000) >> 24) | ((word & 0xff0000) >> 8) | ((word & 0xff00) << 8) | ((word & 0xff) << 24);
        logger->trace(" 0x{:08x}", word);

        tx_core.writeFifo(word);
      }

      ibyte += header.length;

      tx_core.releaseFifo();
    } // end of while (ibyte < msg_data.size())

    if (logger->should_log(spdlog::level::debug)) {
      auto data = std::string((const char*)msg_data.data(), msg.size());

      logger->debug(">>> message from {}:{}, size={}", ep.address(), ep.port(), msg.size());

      std::stringstream ss;
      for(unsigned i = 0; i < data.size(); i++) {
        if(i%32 == 0) {
          if(i != 0) {
            logger->debug("{}", ss.str());
            ss.str("");
          }
          ss << " 0x" << std::hex << std::setfill('0') << std::setw(8) << i << ": ";
        }
        ss << std::hex << std::setfill('0') << std::setw(2) << ((unsigned int)data[i] & 0xFF) << " " << std::dec;
      }
      logger->debug("{}", ss.str());
    }
  }
};

class Sender {
public:
  virtual ~Sender() {}
  virtual void send(const netio::message& msg) = 0;
  virtual void flush() = 0;
  virtual void reset_tags() {}
  virtual void add_tag(netio::tag) {}
};

class SenderHT : public Sender {
  netio::buffered_send_socket bss;
public:
  SenderHT(netio::context* ctx, std::string host, unsigned short port)
    : bss(ctx) {
    logger->info("Set up SenderHT to {}:{}", host, port);
    bss.connect(netio::endpoint(host.c_str(), port));
  }

  void send(const netio::message& msg) {
    bss.send(msg);
  }

  void flush() {
    bss.flush();
  }
};

class SenderLL : public Sender {
  netio::low_latency_send_socket llss;
public:
  SenderLL(netio::context* ctx, std::string host, unsigned short port)
    : llss(ctx) {
    logger->info("Set up SenderLL to {}:{}", host, port);
    llss.connect(netio::endpoint(host.c_str(), port));
  }

  void send(const netio::message& msg) {
    llss.send(msg);
  }

  void flush() {
  }
};

class SenderPublish : public Sender {
  netio::publish_socket socket;
  std::vector<netio::tag> tags;
public:
  SenderPublish(netio::context* ctx, std::vector<netio::tag>& tags, unsigned short port)
    : socket(ctx, port), tags(tags) {
    logger->info("Set up SenderPublish with port {}", port);
  }

  void reset_tags() {tags.clear();}
  void add_tag(netio::tag newtag) {tags.push_back(newtag);}

  void send(const netio::message& msg) {
    for(unsigned i=0; i<tags.size(); i++) {
      socket.publish(tags[i], msg);
    }
  }

  void flush() {}
};

static void send_loop(Sender& s, input_reader& in)
{
  logger->info("Start sender loop");

  while(in.data_available()) {
    netio::message msg;
    netio::tag tag; // elink
    in.next_message(msg, tag);
    if(msg.size() == 0) continue;

    logger->trace("Send message length {}", msg.size());

    s.reset_tags();
    s.add_tag(tag);

    s.send(msg);
  }

  logger->info("End sender loop");

  s.flush();
}

typedef std::function<void (netio::endpoint& ep, netio::message& msg)> ReceiveFunc;

class Receiver {
public:
  Receiver() {}
  virtual ~Receiver() {};
  virtual void run(ReceiveFunc f) = 0;
};

class ReceiverLL : public Receiver {
  netio::context* ctx;
  unsigned short port;

public:
  ReceiverLL(netio::context* ctx, unsigned short port)
    : ctx(ctx), port(port)
  {
    logger->info("Setup ReceiverLL on port {}", port);
  }

  void run(ReceiveFunc f) {
    netio::sockcfg cfg = netio::sockcfg::cfg();
    if(app_settings.zerocopy)
      cfg(netio::sockcfg::ZERO_COPY);

    netio::low_latency_recv_socket socket
      (ctx, port,
       [&](netio::endpoint& ep, netio::message& msg) {
          f(ep, msg);
      }, cfg);

    logger->info("ReceiverLL waiting");
    while(true) {
      usleep(100*1000);
    }
    logger->info("  ReceiverLL done waiting!");
  }
};

class ReceiverHT : public Receiver {
  netio::context* ctx;
  unsigned short port;
public:
  ReceiverHT(netio::context* ctx, unsigned short port)
    : ctx(ctx), port(port)
  {
    logger->info("Setup ReceiverHT on port {}", port);
  }

  void run(ReceiveFunc f) {
    logger->info("Setup ReceiverHT");

    netio::sockcfg cfg = netio::sockcfg::cfg();
    if(app_settings.zerocopy)
      cfg(netio::sockcfg::ZERO_COPY);

    try {
      netio::recv_socket socket(ctx, port, cfg);
      netio::endpoint ep;
      netio::message msg;

      logger->info("Entering ReceiverHT loop");
      while(true) {
        socket.recv(ep, msg);
        logger->debug("  ReceiverHT loop recv");
        f(ep, msg);
      }
    } catch(std::runtime_error &re) {
      logger->error("Failed to set up NetIO receiver on port {}", port);
      logger->error("{}", re.what());
      throw;
    }

    logger->info("Finished ReceiverHT");
  }
};

class ReceiverSubscribeLL : public Receiver {
  netio::context* ctx;
  std::string host;
  unsigned short port;
  std::vector<netio::tag> tags;
public:
  ReceiverSubscribeLL(netio::context* ctx,
                      std::string host,
                      unsigned short port, std::vector<netio::tag> tags)
    : ctx(ctx), host(host), port(port), tags(tags) {}

  void run(ReceiveFunc f) {
    logger->info("Setup ReceiverSubscribeLL");

    netio::sockcfg cfg = netio::sockcfg::cfg();
    if(app_settings.zerocopy)
    cfg(netio::sockcfg::ZERO_COPY);

    netio::low_latency_subscribe_socket socket
      (ctx,
       [&](netio::endpoint& ep, netio::message& msg) {
           f(ep, msg);
         }, cfg);

    for(unsigned i=0; i<tags.size(); i++) {
      try {
        socket.subscribe(tags[i], netio::endpoint(host, port));
      } catch(std::runtime_error &re) {
        logger->error("Failed to subscribe to {}:{} (LL) for tag {}", host, port, tags[i]);
        logger->error("{}", re.what());
      }
    }

    logger->info("Waiting in ReceiverSubscribeLL");

    while(true) {
      usleep(100*1000);
    }
  }
};

class ReceiverSubscribeHT : public Receiver {
  netio::context* ctx;
  std::string host;
  unsigned short port;
  std::vector<netio::tag> tags;

public:
  ReceiverSubscribeHT(netio::context* ctx, std::string host,
                      unsigned short port, std::vector<netio::tag> tags)
    : ctx(ctx), host(host), port(port), tags(tags) {}

  void run(ReceiveFunc f) {
    logger->info("Setup ReceiverSubscribeHT");

    netio::sockcfg cfg = netio::sockcfg::cfg();
    if(app_settings.zerocopy)
      cfg(netio::sockcfg::ZERO_COPY);

    netio::subscribe_socket socket(ctx, cfg);
    for(unsigned i=0; i<tags.size(); i++) {
      try {
        socket.subscribe(tags[i], netio::endpoint(host, port));
      } catch(std::runtime_error &re) {
        logger->error("Failed to subscribe to {}:{} (HT) for tag {}", host, port, tags[i]);
        logger->error("{}", re.what());
      }
    }

    netio::endpoint ep;
    netio::message msg;

    logger->info("ReceiverSubscribeHT: Waiting");

    while(true) {
      socket.recv(ep, msg);
      f(ep, msg);
    }
  }
};

void help()
{
  std::cout << "Use NetIO to set up a server running a YARR controller\n";
  std::cout << "Options:\n";
  std::cout << "\t-p=NUM\tPort number to listen for connections on (from NetioTxCore)\n";
  std::cout << "\t-q=NUM\tPort number to send to (to NetioRxCore)\n";
  std::cout << "\t-f=FILE\tConfiguration of controller to use (defaults to emu)\n";
  std::cout << "\t-b=BACKEND\tSpecify NetIO backend (default posix)\n";
  std::cout << "\t-z\tUse zero-copy NetIO sockets\n";
  std::cout << "\t-t\tUse NetIO in high-throughput mode\n";
  std::cout << "\t-v\tIncrease verbosity (use twice for information on every exchange, three times for more info\n";
  std::cout << "\t-l=LOGCONFIG\tLogger configurations (overwrite -v if provided)\n";
  std::cout << "\t-H=HOST Specify Host to connect to (not implemented)\n";
  std::cout << "\t-h\tReport this usage info\n";
  std::cout << "\n";
}

int main(int argc, char** argv)
{
  // Defaults, NetioTxCore sends to 12340 and NetioRxCore recvs from 12350
  unsigned short rxPort = 12340;
  unsigned short txPort = 12350;

  // Potentially remote could be different for tx and rx
  std::string host = "127.0.0.1";
  std::string backend = "posix";
  std::string fname = "configs/controller/emuCfg.json";

  socket_type sockettype = SOCKET_LL;

  // logger config
  std::string logCfgPath = "";

  char opt;
  std::string temp;
  while ((opt = getopt(argc, argv, "b:H:p:q:f:l:zthv")) != -1) {
    switch (opt) {
    case 'b':
      backend = optarg;
      break;
    case 'H':
      host = optarg;
      break;
    case 'p':
      rxPort = atoi(optarg);
      break;
    case 'q':
      txPort = atoi(optarg);
      break;
    case 'f':
      fname = optarg;
      break;
    case 'z':
      app_settings.zerocopy = true;
      break;
    case 't':
      sockettype = SOCKET_HT;
      break;
    case 'l':
      logCfgPath = std::string(optarg);
    case 'v':
      app_settings.verbosity ++;
      break;
    case 'h':
    default:
      help();
      return -1;
    }
  }

  spdlog::info("Configuring logger ...");
  if (logCfgPath.empty()) {
    // default setting
    json j;
    std::string defaultLogPattern = "[%T:%e]%^[%=8l][%=15n]:%$ %v";
    j["pattern"] = defaultLogPattern;
    j["log_config"][0]["name"] = "netio_bridge";
    if (app_settings.verbosity > 2) {
      j["log_config"][0]["level"] = "trace";
    } else if (app_settings.verbosity > 1) {
      j["log_config"][0]["level"] = "debug";
    } else {
      j["log_config"][0]["level"] = "info";
    }
    logging::setupLoggers(j);
  } else {
    try {
      auto j = ScanHelper::openJsonFile(logCfgPath);
      logging::setupLoggers(j);
    } catch (std::runtime_error &e) {
      spdlog::error("Failed to load logger config: {}", e.what());
      return -1;
    }
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

  netio::context ctx(backend.c_str());
  std::thread bg_thread([&ctx]() {
      logger->info("Start NetIO processing thread");
      ctx.event_loop()->run_forever();
      logger->info("Netio processing thread done");
    });

  std::unique_ptr<Sender> sender;
  std::unique_ptr<Receiver> receiver;

  bool do_publish = true;
  bool do_subscribe = false;
  std::vector<netio::tag> tags;
  // Elinks
  tags.push_back(0);

  if(do_publish) {
    sender.reset(new SenderPublish(&ctx, tags, txPort));
  } else if (sockettype == SOCKET_LL) {
    sender.reset(new SenderLL(&ctx, host, txPort));
  } else if(sockettype == SOCKET_HT) {
    sender.reset(new SenderHT(&ctx, host, txPort));
  }

  if(do_subscribe) {
    if(sockettype == SOCKET_LL) {
      receiver.reset(new ReceiverSubscribeLL(&ctx, host, rxPort, tags));
    } else if(sockettype == SOCKET_HT) {
      receiver.reset(new ReceiverSubscribeHT(&ctx, host, rxPort, tags));
    }
  } else if (sockettype == SOCKET_LL) {
    receiver.reset(new ReceiverLL(&ctx, rxPort));
  } else if(sockettype == SOCKET_HT) {
    receiver.reset(new ReceiverHT(&ctx, rxPort));
  } else {
    logger->error("No socket type selected");
    exit(1);
  }

  std::thread reader_thread([&sender, &hwCtrl]() {
      input_reader in(*hwCtrl);
      send_loop(*sender, in);
      logger->info("Finish send loop");
    });

  std::thread writer_thread([&receiver, &hwCtrl]() {
      output_writer out(*hwCtrl);
      receiver->run([&out](netio::endpoint& ep, netio::message& msg) {
          logger->debug("Main: Process packet");
          out.process_message(ep, msg);
        });
      logger->info("Finish receive loop");
    });

  reader_thread.join();

  ctx.event_loop()->stop();
  bg_thread.join();

  writer_thread.join();

  return 0;
}
