#include <bitset>
#include <iostream>
#include <functional>

#include "SpecController.h"
#include "AllHwControllers.h"
#include "StarCmd.h"
#include "LCBUtils.h"
#include "LoggingConfig.h"
#include "ScanHelper.h"
#include "StarChipPacket.h"
#include "logging.h"
#include "LoopStatus.h"

namespace {
  auto logger = logging::make_log("test_star");

  StarCmd star;

  struct Hybrid {
     uint32_t tx; uint32_t rx; uint32_t hcc_id; std::vector<uint32_t> abc_id;
  };
}

static void printHelp();

void sendCommand(const std::array<uint16_t, 9>& cmd, HwController& hwCtrl);
void sendCommand(uint16_t cmd, HwController& hwCtrl);
void configureChips(StarCmd &star, HwController& hwCtrl, bool doReset);
void runTests(StarCmd &star, HwController& hwCtrl, bool hprOff);
void reportData(RawData &data, bool do_spec_specific=false);
int packetFromRawData(StarChipPacket& packet, RawData& data);
std::unique_ptr<RawData, void(*)(RawData*)> readData(HwController&, std::function<bool(RawData&)>, uint32_t timeout=1000);
RawDataContainer readAllData(HwController&, std::function<bool(RawData&)>, uint32_t timeout=2000);
// Data filters
bool isFromChannel(RawData& data, uint32_t chn);
bool isPacketType(RawData& data, PacketType packet_type);
//
bool checkHPRs(HwController& hwCtrl, const std::vector<uint32_t>& rxChannels);
std::vector<Hybrid> probeHCCs(HwController& hwCtrl, const std::vector<uint32_t>& txChannels, const std::vector<uint32_t>& rxChannels, bool setID);
void configureHCC(HwController& hwCtrl, bool doReset);
bool probeABCs(HwController& hwCtrl, std::vector<Hybrid>& hccStars);
void configureABC(HwController& hwCtrl, bool doReset);
bool testHCCRegisterAccess(HwController& hwCtrl, std::vector<Hybrid>& hccStars);
bool testABCRegisterAccess(HwController& hwCtrl, std::vector<Hybrid>& hccStars);

int main(int argc, char *argv[]) {
    std::string controller;
    std::string controllerType;

    {
      json j; // Start empty
      std::string defaultLogPattern = "[%T:%e]%^[%=8l][%=15n]:%$ %v";
      j["pattern"] = defaultLogPattern;
      j["log_config"][0]["name"] = "all";
      j["log_config"][0]["level"] = "info";
      logging::setupLoggers(j);
    }

    // Original Spec version
    std::vector<uint32_t> rxChannels = {6};
    std::vector<uint32_t> txChannels = {0xFFFF};
    bool setHccId = false;
    int c;
    while ((c = getopt(argc, argv, "hl:r:t:d")) != -1) {
      switch(c) {
      case 'h':
        printHelp();
        return 0;
      case 'l':
        try {
          std::string logPath = std::string(optarg);
          auto j = ScanHelper::openJsonFile(logPath);
          logging::setupLoggers(j);
        } catch (std::runtime_error &e) {
          spdlog::error("Opening logger config: {}", e.what());
          return 1;
        }
        break;
      case 'r':
        rxChannels.clear();
        optind -= 1;
        for (; optind < argc && *argv[optind] != '-'; optind += 1) {
          rxChannels.push_back( atoi(argv[optind]) );
        }
        break;
      case 't':
        txChannels.clear();
        optind -= 1;
        for (; optind < argc && *argv[optind] != '-'; optind += 1) {
          txChannels.push_back( atoi(argv[optind]) );
        }
        break;
      case 'd':
        setHccId = true;
        break;
      default:
        spdlog::critical("Error while parsing command line parameters!");
        return -1;
      }
    }

    if (optind != argc) {
      // First positional parameter (optind is first not parsed by getopt)
      controller = argv[optind];
    }
    std::cout << std::endl;
    std::unique_ptr<HwController> hwCtrl = nullptr;
    if(controller.empty()) {
	controllerType = "spec";
        hwCtrl = StdDict::getHwController(controllerType);
        // hwCtrl->init(0);
    } else {
      try {
        std::cout << " Using controller from " << controller << "\n";
        json ctrlCfg = ScanHelper::openJsonFile(controller);
        controllerType = ctrlCfg["ctrlCfg"]["type"];
        hwCtrl = StdDict::getHwController(controllerType);
        hwCtrl->loadConfig(ctrlCfg["ctrlCfg"]["cfg"]);
      } catch (std::runtime_error &e) {
        spdlog::error("Opening controller config: {}", e.what());
        return 1;
      }
    }

    hwCtrl->toggleTrigAbort();
    hwCtrl->setTrigEnable(0);

    // In fact, mostly needed only for a specific test version of Spec FW
    bool do_spec_specific = controllerType == "spec";

    if(do_spec_specific) {
      //Send IO config to active FMC
      SpecController &s = *dynamic_cast<SpecController*>(&*hwCtrl);
      s.writeSingle(0x6<<14 | 0x0, 0x9ce730);
      s.writeSingle(0x6<<14 | 0x1, 0xF);
    }

    // Enable Tx channels
    hwCtrl->setCmdEnable(txChannels);

    // Enable Rx channels
    hwCtrl->disableRx();
    hwCtrl->setRxEnable(rxChannels);

    //////////
    // Reset
    sendCommand( LCB::fast_command(LCB::LOGIC_RESET, 0), *hwCtrl );
    sendCommand( LCB::fast_command(LCB::HCC_REG_RESET, 0), *hwCtrl );
    sendCommand( LCB::fast_command(LCB::ABC_REG_RESET, 0), *hwCtrl );

    //////////
    // Read HCCStar HPRs
    if (controllerType == "emu_Star") {
      // For the emulator, toggle the TestHPR bit
      sendCommand(star.write_hcc_register(16, 0x2), *hwCtrl);
    }

    bool hprOK = checkHPRs(*hwCtrl, rxChannels);
    if (not hprOK)
      return 1;

    //////////
    // Probe HCCs
    auto hccStars = probeHCCs(*hwCtrl, txChannels, rxChannels, setHccId);
    if (hccStars.empty())
      return 1;

    //////////
    // Test HCCStar register read and write
    bool hccRegOk = testHCCRegisterAccess(*hwCtrl, hccStars);
    if (not hccRegOk)
      return 1;

    //////////
    // Configure HCCs to enable communications with ABCs
    configureHCC(*hwCtrl, true);

    // Read HPRs from ABCStars
    if (controllerType == "emu_Star") {
      // For the emulator, toggle the TestHPR bit
      sendCommand(star.write_abc_register(0, 0x8), *hwCtrl);
    }

    bool abcCommUp = probeABCs(*hwCtrl, hccStars);
    if (not abcCommUp)
      return 1;

    //////////
    // Test ABCStar register read and write
    bool abcRegOk = testABCRegisterAccess(*hwCtrl, hccStars);
    if (not abcRegOk)
      return 1;

    //////////
    // Configure ABCs
    configureABC(*hwCtrl, true);

    // For now
    //configureChips(star, *hwCtrl, true);
    runTests(star, *hwCtrl, true);

    uint32_t timeout = 2000;
    auto rdc = readAllData(
      *hwCtrl,
      [](RawData& d){
        return not(isPacketType(d,TYP_HCC_HPR) or isPacketType(d,TYP_ABC_HPR));
      },
      timeout
      );

    for (unsigned c = 0; c < rdc.size(); c++) {
      RawData d(rdc.adr[c], rdc.buf[c], rdc.words[c]);
      reportData(d, controllerType == "spec");
    }

    hwCtrl->disableRx();

    if (rdc.size() == 0)
      return 1;

    return 0;
}

void printHelp() {
  std::cout << "Usage: test_star HW_CONFIG [OPTIONS] ... \n";
  std::cout << "   Run Star FE tests with HardwareController configuration from HW_CONFIG\n";
  std::cout << " -h: Show this help.\n";
  std::cout << " -r <channel1> [<channel2> ...] : Rx channels to enable. Can take multiple arguments.\n";
  std::cout << " -t <channel1> [<channel2> ...] : Tx channels to enable. Can take multiple arguments.\n";
  std::cout << " -l <log_config> : Configure loggers.\n";
  std::cout << " -d : Modify HCCStar IDs when probing.\n";
}

void runTests(StarCmd &star, HwController& hwCtrl, bool hprOff) {

  if (hprOff) {
    // Turn off HCC HPR
    sendCommand( star.write_hcc_register(43, 0x00000100), hwCtrl);
    sendCommand( star.write_hcc_register(16, 0x00000001), hwCtrl);

    // Turn off ABC HPR
    sendCommand( star.write_abc_register(32, 0x00000740), hwCtrl);
    sendCommand( star.write_abc_register(0, 0x00000004), hwCtrl);
  }

  //////////
  // Read HCCStar registers
  // register 17: Addressing
  sendCommand( star.read_hcc_register(17), hwCtrl);
  // register 40: ICenable
  sendCommand( star.read_hcc_register(40), hwCtrl);

  //////////
  // Read ABCStar registers
  // register 32: CREG0
  sendCommand( star.read_abc_register(32), hwCtrl);
  // register 34: CREG2
  sendCommand( star.read_abc_register(34), hwCtrl);

  //////////
  // Read data packets

  // Set some mask registers
  sendCommand( star.write_abc_register(19, 0xfffe0000), hwCtrl); // MaskInput3
  sendCommand( star.write_abc_register(23, 0xfffe0000), hwCtrl); // MaskInput7

  // Enable hit counters
  sendCommand( star.write_abc_register(32, 0x00000760), hwCtrl);

  // Reset and start ABCStar hit counters
  sendCommand(LCB::fast_command(LCB::ABC_HIT_COUNT_RESET, 0), hwCtrl);
  sendCommand(LCB::fast_command(LCB::ABC_HIT_COUNT_START, 0), hwCtrl);
  // Enable PR & LP
  sendCommand(LCB::fast_command(LCB::HCC_START_PRLP, 0), hwCtrl);
  // BC Reset
  sendCommand(LCB::lonely_bcr(), hwCtrl);

  /////
  // Static test mode: TM = 1
  sendCommand( star.write_abc_register(32, 0x00010760), hwCtrl);

  // Send a trigger
  sendCommand( LCB::l0a_mask(1, 42, false), hwCtrl);

  /////
  // Test pulse mode: TM = 2
  // Single digital pulse: TestPulseEnable = 1, TestPattEnable = 0
  sendCommand( star.write_abc_register(32, 0x00020770), hwCtrl);

  // Set the L0 pipeline latency to a smaller value: 15 BC
  sendCommand( star.write_abc_register(34, 0x0000000f), hwCtrl);

  // Also set BCIDrstDelay in the HCC to L0 latency - 2 to avoid BCID errors
  sendCommand( star.write_hcc_register(44, 0x0000000d), hwCtrl);

  sendCommand( LCB::lonely_bcr(), hwCtrl);

  // Send a digital pulse followed by a trigger
  std::array<uint16_t, 9> cmd = {
    LCB::IDLE, LCB::IDLE,
    LCB::fast_command(LCB::ABC_DIGITAL_PULSE, 0), LCB::IDLE,
    LCB::IDLE, LCB::IDLE,
    LCB::l0a_mask(1, 43, false), LCB::IDLE,
    LCB::IDLE
  };
  sendCommand(cmd, hwCtrl);

  // Read a ABCStar hit count
  // register 191: HitCountREG63
  sendCommand(star.read_abc_register(191), hwCtrl);
}

int packetFromRawData(StarChipPacket& packet, RawData& data) {
  packet.clear();

  packet.add_word(0x13C); //add SOP
  for(unsigned iw=0; iw<data.words; iw++) {
    for(int i=0; i<4;i++){
      packet.add_word((data.buf[iw]>>i*8)&0xFF);
    }
  }
  packet.add_word(0x1DC); //add EOP

  return packet.parse();
}

void reportData(RawData &data, bool do_spec_specific) {
  std::cout << "Raw data from RxCore:\n";
  std::cout << data.adr << " " << data.buf << " " << data.words << "\n";

  for (unsigned j=0; j<data.words;j++) {
    auto word = data.buf[j];

    if(do_spec_specific) {
      if((j%2) && (word == 0xd3400000)) continue;
      if(!(j%2) && ((word&0xff) == 0xff)) continue;

      if((word&0xff) == 0x5f) continue;

      if(word == 0x1a0d) continue; // Idle on chan 6
      if(word == 0x19f2) continue; // Idle on chan 6

      word &= 0xffffc3ff; // Strip of channel number
    }

    std::cout << "[" << j << "] = " << std::setfill('0') << std::hex << std::setw(8) << word << std::dec << " " << std::bitset<32>(word) << std::endl;
  }

  StarChipPacket packet;

  if ( packetFromRawData(packet, data) ) {
    std::cout << "Parse error\n";
  } else {
    auto packetType = packet.getType();
    if(packetType == TYP_LP || packetType == TYP_PR) {
      packet.print_clusters(std::cout);
    } else if(packetType == TYP_ABC_RR || packetType == TYP_HCC_RR || packetType == TYP_ABC_HPR || packetType == TYP_HCC_HPR) {
      packet.print_more(std::cout);
    }
  }
}

void sendCommand(const std::array<uint16_t, 9>& cmd, HwController& hwCtrl) {
  hwCtrl.writeFifo((LCB::IDLE << 16) + LCB::IDLE);
  hwCtrl.writeFifo((cmd[0] << 16) + cmd[1]);
  hwCtrl.writeFifo((cmd[2] << 16) + cmd[3]);
  hwCtrl.writeFifo((cmd[4] << 16) + cmd[5]);
  hwCtrl.writeFifo((cmd[6] << 16) + cmd[7]);
  hwCtrl.writeFifo((cmd[8] << 16) + LCB::IDLE);
  hwCtrl.releaseFifo();
}

void sendCommand(uint16_t cmd, HwController& hwCtrl) {
  hwCtrl.writeFifo((LCB::IDLE << 16) + cmd);
  hwCtrl.releaseFifo();
}

std::unique_ptr<RawData, void(*)(RawData*)> readData(
  HwController& hwCtrl,
  std::function<bool(RawData&)> filter_cb,
  uint32_t timeout)
{
  bool nodata = true;

  std::unique_ptr<RawData, void(*)(RawData*)> data(
    hwCtrl.readData(),
    [](RawData* d){delete[] d->buf; delete d;} // deleter
    );

  auto start_reading = std::chrono::steady_clock::now();

  while (true) {
    if (data) {
      nodata = false;
      logger->trace("Use data: {}", (void*)data->buf);

      // check if it is the type of data we want
      bool good = filter_cb(*data);
      if (good) {
        break;
      } else {
        data.reset(nullptr);
      }
    } else { // no data
      // wait a bit
      static const auto SLEEP_TIME = std::chrono::milliseconds(1);
      std::this_thread::sleep_for( SLEEP_TIME );
    }

    auto run_time = std::chrono::steady_clock::now() - start_reading;
    if ( run_time > std::chrono::milliseconds(timeout) ) {
      logger->debug("readData timeout");
      break;
    }

    data.reset(hwCtrl.readData());
  }

  if (nodata) {
    logger->critical("No data");
  } else if (not data) {
    logger->debug("No data met the requirement");
  }

  return data;
}

RawDataContainer readAllData(
  HwController& hwCtrl,
  std::function<bool(RawData&)> filter_cb,
  uint32_t timeout)
{
  bool nodata = true;

  RawDataContainer rdc(LoopStatus::empty());

  std::unique_ptr<RawData, void(*)(RawData*)> data(
    hwCtrl.readData(),
    [](RawData* d){delete[] d->buf; delete d;} // deleter
    );

  auto start_reading = std::chrono::steady_clock::now();

  while (true) {
    if (data) {
      nodata = false;
      logger->trace("Use data: {}", (void*)data->buf);

      bool good = filter_cb(*data);
      if (good) {
        rdc.add(data.release());
      }
    } else {
      // wait a bit if no data
      static const auto SLEEP_TIME = std::chrono::milliseconds(1);
      std::this_thread::sleep_for( SLEEP_TIME );
    }

    auto run_time = std::chrono::steady_clock::now() - start_reading;
    if ( run_time > std::chrono::milliseconds(timeout) ) {
      logger->trace("readData timeout");
      break;
    }

    data.reset(hwCtrl.readData());
  }

  if (nodata) {
    logger->critical("No data");
  } else if (rdc.size() == 0) {
    logger->debug("Data container is empty");
  }

  return rdc;
}

bool isFromChannel(RawData& data, uint32_t chn) {
  return data.adr == chn;
}

bool isPacketType(RawData& data, PacketType packet_type) {
  StarChipPacket packet;
  if ( packetFromRawData(packet, data) ) {
    // failed to parse the packet
    return false;
  } else {
    return packet.getType() == packet_type;
  }
}

bool checkHPRs(HwController& hwCtrl, const std::vector<uint32_t>& rxChannels) {
  uint32_t timeout = 1000; // milliseconds

  bool hprOK = false;

  for (auto rx : rxChannels) {
    logger->info("Reading HPR packets from Rx channel {}", rx);

    std::function<bool(RawData&)> filter_hpr = [rx](RawData& d) {
      return isPacketType(d, TYP_HCC_HPR) and isFromChannel(d, rx);
    };

    auto data = readData(hwCtrl, filter_hpr, timeout);
    if (data) {
      // print
      StarChipPacket packet;
      if (packetFromRawData(packet, *data)) {
        logger->error("Packet parse failed");
      } else {
        std::stringstream os;
        packet.print_more(os);
        std::string str = os.str();
        str.erase(str.end()-1); // strip the extra \n
        logger->info(" Received HPR packet: {}", str);

        //bool r3l1_locked = packet.value & (1<<0);
        //bool pll_locked = packet.value & (1<<5);
        bool lcb_locked = packet.value & (1<<1);
        if (lcb_locked) {
          logger->info(" LCB locked");
          hprOK = true;
        } else {
          logger->error(" LCB NOT locked");
        }
      }
    } else {
      logger->warn(" No HPR packet received from Rx channel {}", rx);
    }

  } // for (auto rx : rxChannels)

  return hprOK;
}

std::vector<Hybrid> probeHCCs(
  HwController& hwCtrl,
  const std::vector<uint32_t>& txChannels,
  const std::vector<uint32_t>& rxChannels,
  bool setID)
{
  std::vector<Hybrid> HCCs;
  uint32_t nHCC = 0;

  for (auto tx : txChannels) {
    bool hasHCCTx = false;

    logger->debug("Broadcast register commands to probe HCC via Tx channel {}", tx);
    hwCtrl.disableCmd();
    hwCtrl.setCmdEnable(tx);

    // Toggle bit 2 in register 16 to load serial number into register 17
    sendCommand(star.write_hcc_register(16, 0x4), hwCtrl);

    // Read the addressing register 17
    sendCommand(star.read_hcc_register(17), hwCtrl);

    // Scan through the Rx channels and look for response
    for (auto rx : rxChannels) {
      auto data = readData(
        hwCtrl,
        [rx](RawData& d) {
          return isPacketType(d, TYP_HCC_RR) and isFromChannel(d, rx);
        }
        );

      if (not data) {
        logger->debug("No response from Rx channel {}", rx);
        continue;
      }

      StarChipPacket packet;
      if (packetFromRawData(packet, *data)) {
        logger->error("Packet parse failed");
      } else {
        uint32_t hccID = (packet.value & 0xf0000000) >> 28; // top four bits
        uint32_t fuseID = packet.value & 0x00ffffff; // lowest 24 bits
        logger->info("Found HCCStar @ Tx = {} Rx = {}: ID = 0x{:x} eFuse ID = 0x{:06x}", tx, rx, hccID, fuseID);
        hasHCCTx = true;

        if (setID) {
          // Set HCC ID to nHCC
          uint32_t hccreg17 = (nHCC << 28) | (fuseID & 0x00ffffff);
          sendCommand(star.write_hcc_register(17, hccreg17), hwCtrl);
          logger->info(" Set its ID to 0x{:x}", nHCC);
          hccID = nHCC;
        }

        Hybrid h{tx, rx, hccID, {}};
        HCCs.push_back(h);

        nHCC++;
      }
    } // end of rx channel loop

    if (not hasHCCTx) {
      logger->warn("No HCCStar on the command segment Tx = {}", tx);
    }
  } // end of tx channel loop

  if (HCCs.empty()) {
    logger->error("No HCCs found");
  }

  return HCCs;
}

void configureHCC(HwController& hwCtrl, bool doReset) {
  // Configure HCCStars to enable communications with ABCStars
  if (doReset) {
    logger->debug("Sending HCCStar register reset command");
    sendCommand(LCB::fast_command(LCB::HCC_REG_RESET, 0), hwCtrl);
  }

  logger->info("Broadcast HCCStar configurations");

  // Register 32 (Delay1): delays for signals to ABCStars
  sendCommand(star.write_hcc_register(32, 0x02400000), hwCtrl);

  // Register 33, 34 (Delay2, Delay3): delays for data from ABCStar
  sendCommand(star.write_hcc_register(33, 0x44444444), hwCtrl);
  sendCommand(star.write_hcc_register(34, 0x00000444), hwCtrl);

  // Register 38 (DRV1): enable driver and currents
  sendCommand(star.write_hcc_register(38, 0x0fffffff), hwCtrl);

  // Register 40 (ICenable): enable input channels
  sendCommand(star.write_hcc_register(40, 0x000007ff), hwCtrl);

  // Register 45/46: external reset for ABCStars
  sendCommand(star.write_hcc_register(45, 0x00000001), hwCtrl);
  sendCommand(star.write_hcc_register(46, 0x00000001), hwCtrl);
}

bool probeABCs(HwController& hwCtrl, std::vector<Hybrid>& hccStars) {
  bool hasABCStar = false;

  for (auto& hcc : hccStars) {
    logger->info("Reading HPR packets from ABCStars on HCCStar {}", hcc.hcc_id);

    unsigned activeInChannels = 0;

    std::function<bool(RawData&)> filter_abchpr = [&hcc](RawData& d) {
      return isPacketType(d, TYP_ABC_HPR) and isFromChannel(d, hcc.rx);
    };
    uint32_t timeout = 1000; // milliseconds

    auto rdc = readAllData(hwCtrl, filter_abchpr, timeout);

    for (unsigned c = 0; c < rdc.size(); c++) {
      RawData d(rdc.adr[c], rdc.buf[c], rdc.words[c]);
      StarChipPacket packet;

      if ( packetFromRawData(packet, d) ) {
        logger->error("Packet parse failed");
      } else {
        logger->trace(" Received an HPR packet from ABCStar");
        hasABCStar = true;

        // check the input channel
        uint32_t abc_chn = packet.channel_abc;
        // FIXME. For now:
        uint32_t abcid = 9 - abc_chn;

        if ( std::find(hcc.abc_id.begin(), hcc.abc_id.end(), abcid) == hcc.abc_id.end() ) {
          // new channel
          hcc.abc_id.push_back(abcid);
          logger->info(" Received an HPR packet from the ABCStar on channel {}", abc_chn);
          activeInChannels |= (1 << abc_chn);

          // print the HPR packet
          std::stringstream os;
          packet.print_more(os);
          std::string str = os.str();
          str.erase(str.end()-1); // strip the extra \n
          logger->info(" Received HPR packet: {}", str);
        }
      }

    } // end of data container loop

    if (not activeInChannels) {
      logger->warn("No ABCStar data from HCCStar {}", hcc.hcc_id);
    }

    // Update HCC register 40 ICenable
    logger->info(" Set register 40 (ICenable) on HCCStar {} to 0x{:08x}", hcc.hcc_id, activeInChannels);
    sendCommand(star.write_hcc_register(40, activeInChannels, hcc.hcc_id), hwCtrl);
  } // end of HCC loop

  if (not hasABCStar) {
    logger->error("No ABCStar from any HCCStar");
  }

  return hasABCStar;
}

void configureABC(HwController& hwCtrl, bool doReset) {

  if (doReset) {
    logger->debug("Sending ABCStar register reset commands");
    sendCommand(LCB::fast_command(LCB::ABC_REG_RESET, 0), hwCtrl);
    sendCommand(LCB::fast_command(LCB::ABC_SLOW_COMMAND_RESET, 0), hwCtrl);
  }

  logger->info("Broadcast ABCStar configurations");

  // Register 32 (CREG0): set RR mode to 1, enable LP and PR
  sendCommand(star.write_abc_register(32, 0x00000700), hwCtrl);
}

bool testRegisterReadWrite(HwController& hwCtrl, uint32_t regAddr, uint32_t write_value, uint32_t rx, int hccId, int abcId=-1) {
  bool isHCC = abcId < 0;

  ////
  // Test register read
  int reg_read_value = -1;

  std::string reg_str;
  PacketType ptype;
  if (isHCC) { // HCC register
    reg_str = "register "+std::to_string(regAddr)+" on HCCStar "+std::to_string(hccId);
    ptype = TYP_HCC_RR;

    logger->info("Reading "+reg_str);

    // Send register read command
    sendCommand(star.read_hcc_register(regAddr, hccId), hwCtrl);

  } else { // ABC register
    reg_str = "register "+std::to_string(regAddr)+" on ABCStar @ channel "+std::to_string(abcId)+" of HCCStar "+std::to_string(hccId);
    ptype = TYP_ABC_RR;

    logger->info("Reading "+reg_str);

    // Send register read command
    sendCommand(star.read_abc_register(regAddr, hccId, abcId), hwCtrl);
  }

  // Read data
  auto data = readData(
    hwCtrl,
    [&](RawData& d) {return isPacketType(d, ptype) and isFromChannel(d, rx);}
    );

  if (data) {
    StarChipPacket packet;
    packetFromRawData(packet, *data);
    reg_read_value = packet.value;

    if (logger->should_log(spdlog::level::debug)) {
      // print packet
      std::stringstream os;
      packet.print_more(os);
      std::string str = os.str();
      str.erase(str.end()-1); // strip the extra \n
      logger->debug(" Received RR packet: {}", str);
    }
    logger->info("Register read: OK");
  } else {
    logger->error("Register read: Fail");
    return false;
  }

  ////
  // Test register write
  // Write a new value to the same register
  // Read the register and check its value is updated
  assert(write_value != reg_read_value);

  logger->info("Writing value 0x{:08x} to {}", write_value, reg_str);
  if (isHCC) {
    sendCommand(star.write_hcc_register(regAddr, write_value, hccId), hwCtrl);
    logger->info("Reading "+reg_str);
    sendCommand(star.read_hcc_register(regAddr, hccId), hwCtrl);
  } else {
    sendCommand(star.write_abc_register(regAddr, write_value, hccId, abcId), hwCtrl);
    logger->info("Reading "+reg_str);
    sendCommand(star.read_abc_register(regAddr, hccId, abcId), hwCtrl);
  }

  // Read data
  auto wdata = readData(
    hwCtrl,
    [&](RawData& d) {return isPacketType(d, ptype) and isFromChannel(d, rx);}
    );

  if (wdata) {
    StarChipPacket wpacket;
    packetFromRawData(wpacket, *wdata);

    if (logger->should_log(spdlog::level::debug)) {
      // print packet
      std::stringstream os;
      wpacket.print_more(os);
      std::string str = os.str();
      str.erase(str.end()-1); // strip the extra \n
      logger->debug(" Received RR packet: {}", str);
    }

    // check if the value is what we wrote
    if (wpacket.value == write_value) {
      logger->info("Register write: OK");
    } else {
      logger->error("Register write: Fail");
      logger->error("The value read back from register {} is: 0x{:08x}", regAddr, wpacket.value);
      return false;
    }
  } else {
    logger->error("Failed to read data");
    return false;
  }

  return true;
}

bool testHCCRegisterAccess(HwController& hwCtrl, std::vector<Hybrid>& hccStars) {
  logger->info("Test HCCStar register read & write");

  bool success = true;

  for (auto& hcc : hccStars) {
    // Register 47: ErrCfg
    success &= testRegisterReadWrite(hwCtrl, 47, 0xdeadbeef, hcc.rx, hcc.hcc_id);
  }

  return success;
}

bool testABCRegisterAccess(HwController& hwCtrl, std::vector<Hybrid>& hccStars) {
  logger->info("Test ABCStar register read & write");

  // Set RR mode to 1
  sendCommand(star.write_abc_register(32, 0x00000400), hwCtrl);

  bool success = true;

  for (auto& hcc : hccStars) {
    for (uint32_t iabc : hcc.abc_id) {
      // Register 16: MaskInput0
      success &= testRegisterReadWrite(hwCtrl, 16, 0xabadcafe, hcc.rx, hcc.hcc_id, iabc);
    }
  }

  return success;
}
