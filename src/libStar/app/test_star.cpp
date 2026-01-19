#include <bitset>
#include <iostream>
#include <functional>
#include <tuple>
#include <set>

#include "AbcNames.h"
#include "AllHwControllers.h"
#include "HccNames.h"
#include "StarCmd.h"
#include "StarCfg.h"
#include "StarConstants.h"
#include "LCBUtils.h"
#include "LoggingConfig.h"
#include "ScanHelper.h"
#include "StarChipPacket.h"
#include "logging.h"
#include "LoopStatus.h"

#include <getopt.h>

#include "spdlog/fmt/fmt.h"

namespace {
  auto logger = logging::make_log("test_star");
  // A second logger for recording HCC and ABC IDs after probing
  auto logger_id = logging::make_log("test_star_ids");

  StarCmd star;

  struct Hybrid {
    uint32_t tx;
    uint32_t rx;
    uint32_t hcc_id;
    std::map<uint32_t,uint32_t> abcs; // key: channel; value: chipID
  };

  static const std::map<uint32_t,uint32_t> BROADCAST_ABCS{{15, 15}};

  /**
     Map of sequences to lists of tests.

     Sequences (upper case first letter) are made up of tests (lower case
     first letter).

     NB this is where printHelp gets it's list from. There is a
     cross-verification later that checks that all the known tests are
     part of a sequence (and that sequences are made up of known tests.
  */
  std::map<std::string, std::vector<std::string>> sequenceMap = {
      {"Full", {
          "checkHCCHPRs", "probeHCCs", "testHCCRegister",
          "configureHCC",
          "checkABCHPRs", "probeABCs", "testABCRegister",
          "configureABC",
          "testHitCounts",
          "testDataPacketsStatic",
          "testDataPacketsPulse",
        }},

      /*
        Just test register read and write
      */
      {"Register", {
          "probeHCCs", "testHCCRegister",
          "configureHCC",
          "probeABCs", "testABCRegister",
        }},

      /*
        Read regsiters
      */
      {"MoreRegisters", {
          "readMoreHCCRegisters",
          "readMoreABCRegisters",
        }},

      /*
        Test reading data packets
      */
      {"DataPacket", {
          "probeHCCs", "configureHCC",
          "probeABCs", "configureABC",
          "testDataPacketsStatic",
          "testDataPacketsPulse",
        }},

      /*
        Probe the front end ASICs
      */
      {"Probe", {
          "checkHCCHPRs", "probeHCCs",
          // In case resets were sent, HCCs need to be reconfigured to talk to ABCs
          "configureHCCIfReset", // only if doResets set
          "checkABCHPRs", "probeABCs",
        }},

      /*
        Run some tests in packet transparent mode
      */
      {"PacketTransp", {
          "configureHCCForPacketTransp",
          "configureABC",
          "readABCRegisters",
          "testDataPacketsStatic",
        }},

      /*
        Run some tests in full transparent mode
      */
      {"FullTransp", {
          "configureHCCForFullTransp",
          "configureABC",
          "readABCRegisters",
          "testDataPacketsStatic",

          // "diagnosticsReport",
        }},

      // Read all data (partly here so diagnosticsReport has somewhere to live)
      {"ReadData", {
          "diagnosticsReport",
        }},
    };

  /**
   * Implementation of test actions.
   *
   * Also stores common data that is passed between actions.
   */
  struct TestData {
    unsigned inChannel = 0;

    /// All expected IC enables.
    unsigned icEnablesMask = 0x7ff;

    /// Timeout for read operations
    unsigned timeout_ms = 100;

    std::unique_ptr<StarCfg> starCfg;
    bool setHccId = false;
    bool doResets = false;
    /// Default to 640
    bool mode640 = true;

    // Original Spec version
    std::vector<uint32_t> rxChannels = {6};
    std::vector<uint32_t> txChannels = {0xFFFF};

    std::vector<Hybrid> hccStars;

    std::map<std::string, std::function<bool (HwController&)>> buildTests();

    /// Report on things carried between test actions
    void report() {
      logger->info("Report inter-action config");
      logger->info(" Selected input channel: {}", inChannel);

      logger->info(" HCC ID: {}", starCfg->getHCCchipID());
      for(unsigned i=0; i<Star::MaxABCsPerHCC; i++) {
        if(!(starCfg->isAbcForInputChannel(i))) {
          continue;
        }
        const auto &abc = starCfg->abcForInputChannel(i);

        logger->info("  For IC {}: {}", i, abc.getABCchipID());
      }

      logger->info(" Hybrid tx rx hcc (chan,abcID)");
      for(auto &h: hccStars) {
        std::string abcs;
        for(auto &a: h.abcs) {
          abcs += fmt::format(" ({}, {})", a.first, a.second);
        }
        logger->info("  {} {} {} {}", h.tx, h.rx, h.hcc_id, abcs);
      }
    }

    bool crossCheck();

    /// Update things based on overall provided configuration
    void validate() {
      if(hccStars.empty()) {
        logger->debug("No hybrid configuration, start with broadcast");

        for(auto t: txChannels) {
          for(auto r: rxChannels) {
            logger->debug("Speculative read-write pair TX {} RX {}", t, r);
            Hybrid h{t, r, 15, BROADCAST_ABCS};
            hccStars.push_back(h);
          }
        }
      }
    }
  };

void printHelp() {
  std::cout << "Usage: test_star HW_CONFIG [OPTIONS] ... \n";
  std::cout << "   Run Star FE tests with HardwareController configuration from HW_CONFIG\n";
  std::cout << " -h: Show this help.\n";
  std::cout << " -r <channel1> [<channel2> ...] : Rx channels to enable. Can take multiple arguments.\n";
  std::cout << " -t <channel1> [<channel2> ...] : Tx channels to enable. Can take multiple arguments.\n";
  std::cout << " -l <log_config> : Configure loggers.\n";
  std::cout << " -d : Modify HCCStar IDs when probing.\n";
  std::cout << " -i : All expected ICs (default all: 0x7ff).\n";
  std::cout << " -3 : Configure HCC for 320 instead of 640 (default).\n";
  std::cout << " -R : Send reset commands.\n";
  std::cout << " -v : Report carried data between actions (diagnostic).\n";
  std::cout << " -w <timeout_ms> : General timeout in milliseconds.\n";
  std::cout << " -s <test_preset> : Type of test (lower case), or sequence (UpperCase) to run, use bad to list. Default: Full\n";
  std::cout << " -c <input channel> : HCC input channel. Only used if HCCs are set to full transparent mode.\n";
  std::cout << " -V <chip_version> : Versions of the HCCStar and ABCStar chips. Possible options are: Star, Star_vH0A0, Star_vH0A1, Star_vH1A1. Default: Star (equivalent to Star_vH0A0)\n";
  std::cout << " -T : Run internal cross-checks.\n";
  std::cout << "\n";
  std::cout << "NB in most cases you can run without setting the enables mask\n";
  std::cout << " Otherwise use the following settings:\n";
  std::cout << "   Barrel: 0x7fe\n";
  std::cout << "   R0H0/1: 0x7f8 / 0x1ff\n";
  std::cout << "   R1H0/1: 0x7fe / 0x7ff\n";
  std::cout << "   R2:     0x7e0\n";
  std::cout << "   R3H0/1: 0x7f0\n";
  std::cout << "   R3H2/3: 0x07f\n";
  std::cout << "   R4:     0x7f8\n";
  std::cout << "   R5:     0x7fc\n";
  std::cout << "\n";

  std::set<std::string> allSequenceTestNames;
  std::cout << "Available sequences\n";
  for(auto &ss: sequenceMap) {
    std::cout << "  " << ss.first << "\n";
    for(auto &tn: ss.second) {
      allSequenceTestNames.insert(tn);
    }
  }

  std::cout << "Available tests\n";
  for(auto &ss: allSequenceTestNames) {
    std::cout << "  " << ss << "\n";
  }
}

// Utilities
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

// Update the sub-register value in the star config
// Return the address and new register value to be sent to the chips
std::tuple<uint32_t, uint32_t> updateHCCSubRegister(HCCStarSubRegister subReg, uint32_t value, StarCfg& cfg) {
  cfg.setHCCSubRegisterValue(subReg, value);

  uint32_t addr = cfg.getHCCSubRegisterParentAddr(subReg);
  uint32_t newValue = cfg.getHCCSubRegisterParentValue(subReg);

  return std::make_tuple(addr, newValue);
}

// Assume the register configuration command is always broadcasted to all chips
std::tuple<uint32_t, uint32_t> updateABCSubRegister(ABCStarSubRegister subReg, uint32_t value, StarCfg& cfg) {
  uint32_t newValue = 0;

  cfg.eachAbc([&](auto &abc) {
      abc.setSubRegisterValue(subReg, value);

      // Only last is stored
      newValue = abc.getSubRegisterParentValue(subReg);
    });

  uint32_t addr = cfg.getABCSubRegisterParentAddr(subReg);

  return std::make_tuple(addr, newValue);
}

// Update the register value in the chip config
// Return the register address in int
uint32_t updateHCCRegister(HCCStarRegister addr, uint32_t value, StarCfg& cfg) {
  cfg.setHCCRegister(addr, value);
  return (int)addr;
}

uint32_t updateABCRegister(ABCStarRegister reg, uint32_t value, StarCfg& cfg) {
  uint32_t addr = (uint32_t)reg;
  cfg.eachAbc([&](auto &abc) {
      abc.setRegisterValue(reg, value);
    });

  return addr;
}

void setOpMode(int packetMode, bool mode640, HwController &hwCtrl, StarCfg& cfg) {
  // Register OPmode/OPmodeC
  // NB 41/42 are the same, so don't need to set ROSPEEDC
  if(mode640) {
    updateHCCSubRegister(HCCStarSubRegister::ROSPEED, 1, cfg);
  }

  auto [addr, val] = updateHCCSubRegister(HCCStarSubRegister::OPMODE, packetMode, cfg);

  // We happen to know that 41 + 1 is 42
  sendCommand(star.write_hcc_register(addr, val), hwCtrl);
  sendCommand(star.write_hcc_register(addr + 1, val), hwCtrl);
}

// Enable Tx and Rx channels that are connected to HCCs
void enableConnectedChannels(HwController& hwCtrl, std::vector<Hybrid>& hccStars) {
  // Turn all channels off first
  hwCtrl.disableCmd();
  hwCtrl.disableRx();

  logger->debug("Setting enables for {} hybrids", hccStars.size());

  if (hccStars.empty())
    return;

  std::set<uint32_t> txChns;
  std::set<uint32_t> rxChns;

  for (auto& hcc : hccStars) {
    logger->debug("Enabling tx {} rx {}", hcc.tx, hcc.rx);
    txChns.insert(hcc.tx);
    rxChns.insert(hcc.rx);
  }

  // Enable Tx
  std::vector<uint32_t> txChns_vec(txChns.begin(), txChns.end());
  hwCtrl.setCmdEnable(txChns_vec);

  // Enable Rx
  std::vector<uint32_t> rxChns_vec(rxChns.begin(), rxChns.end());
  hwCtrl.setRxEnable(rxChns_vec);
}

int packetFromRawData(StarChipPacket& packet, RawData& data) {
  packet.clear();

  std::stringstream ss;
  ss << std::hex << std::setfill('0');

  packet.add_word(0x13C); //add SOP
  for(unsigned iw=0; iw<data.getSize(); iw++) {
    for(int i=0; i<4;i++){
      auto byte = (data[iw]>>i*8) & 0xFF;
      packet.add_word(byte);

      if (logger->should_log(spdlog::level::trace)) {
	ss << ' ' << std::setw(2) << static_cast<unsigned>(byte);
      }
    }
  }
  packet.add_word(0x1DC); //add EOP

  logger->trace("Raw data: {}", ss.str());

  return packet.parse();
}

RawDataPtr readData(
  HwController& hwCtrl,
  std::function<bool(RawData&)> filter_cb,
  uint32_t timeout)
{
  bool nodata = true;
  bool done = false;

  RawDataPtr data;
  std::vector<RawDataPtr> dataVec;

  auto start_reading = std::chrono::steady_clock::now();
  while (not done) {
    dataVec = hwCtrl.readData();

    if (not dataVec.empty()) {
      nodata = false;
      for (const auto& d : dataVec) {
        logger->trace("Use data: {}", (void*)d->getBuf());

        // check if it is the type of data we want
        bool good = filter_cb(*d);
        if (good) {
          data = d;
          done = true;
          break;
        }
      }
    } else { // no data
      // wait a bit
      static const auto SLEEP_TIME = std::chrono::milliseconds(1);
      std::this_thread::sleep_for( SLEEP_TIME );
    }

    auto run_time = std::chrono::steady_clock::now() - start_reading;
    if ( run_time > std::chrono::milliseconds(timeout) ) {
      logger->debug("readData timeout");
      done = true;
    }
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
  //  bool nodata = true;

  RawDataContainer rdc(LoopStatus{});
  
  auto start_reading = std::chrono::steady_clock::now();

  std::vector<RawDataPtr> dataVec;
  while (true) {
      dataVec = hwCtrl.readData();
      if (not dataVec.empty()) {
          for(auto data : dataVec) {
              bool good = filter_cb(*data);
              if (good) {
                  rdc.add(std::move(data));
              }
          }
      } else {
          // wait a bit if no data
          static const auto SLEEP_TIME = std::chrono::milliseconds(1);
          std::this_thread::sleep_for( SLEEP_TIME );
      }

      // Timeout
      auto run_time = std::chrono::steady_clock::now() - start_reading;
      if ( run_time > std::chrono::milliseconds(timeout) ) {
          logger->trace("readData timeout");
          break;
      }
  }

  if (rdc.size() == 0) {
      logger->critical("Data container empty");
  }

  return rdc;
}

void reportData(RawData &data) {
  logger->info(" Raw data from RxCore:");
  logger->info(" {} {:p} {}", data.getAdr(), (void*)data.getBuf(), data.getSize());

  for (unsigned j=0; j<data.getSize();j++) {
    auto word = data[j];

    logger->info(" [{}] = {:08x} {:032b}", j, word, word);
  }

  StarChipPacket packet;

  if ( packetFromRawData(packet, data) ) {
    logger->error("Parse error");
  } else {
    std::stringstream ss;
    auto packetType = packet.getType();
    if(packetType == TYP_LP || packetType == TYP_PR) {
      packet.print_clusters(ss);
    } else if(packetType == TYP_ABC_RR || packetType == TYP_HCC_RR || packetType == TYP_ABC_HPR || packetType == TYP_HCC_HPR) {
      packet.print_more(ss);
    }
    logger->info(ss.str());
  }
}

// Data filters
bool isFromChannel(RawData& data, uint32_t chn) {
  return data.getAdr() == chn;
}

bool isPacketType(RawData& data, PacketType packet_type, bool isPacketTransp=false) {
  StarChipPacket packet;
  if ( packetFromRawData(packet, data) ) {
    // failed to parse the packet
    return false;
  } else {
    // successfully parsed the packet

    if (packet.getType() == TYP_ABC_TRANSP) {
      logger->debug("Received a Packet Transparent packet");
      // Check the type of the forwarded ABCStar packet
      // First byte is TYP_ABC_TRANSP and channel number
      // Type of the ABCStar packet is the top 4 bits of the second byte
      int raw_type_abc = (data[0] & 0xf000) >> 12;
      if ( packet_type_headers.find(raw_type_abc) == packet_type_headers.end() ) {
        logger->error("Packet type was parsed as {}, which is an invalid type.", raw_type_abc);
        return false;
      }
      // compare the forwarded ABC packet type to the expected type
      return packet_type_headers[ raw_type_abc ] == packet_type;

    } else {
      // compare the packet type to the expected type
      return packet.getType() == packet_type;
    }
  }
}

// Configure chips
// Different register values for different HCCStar versions?
void configureHCC(HwController& hwCtrl, StarCfg& cfg, bool reset, uint32_t ic_enables, bool mode640) {
  // Configure HCCStars to enable communications with ABCStars
  if (reset) {
    logger->info("Sending HCCStar register reset command");
    sendCommand(LCB::fast_command(LCB::HCC_REG_RESET, 0), hwCtrl);
  }

  logger->info("Broadcast HCCStar configurations");

  // Register Delay1: delays for signals to ABCStars
  uint32_t val_delay1 = 0x02900020;
  uint32_t addr_delay1 = updateHCCRegister(HCCStarRegister::Delay1, val_delay1, cfg);
  sendCommand(star.write_hcc_register(addr_delay1, val_delay1), hwCtrl);

  // Register Delay2, Delay3: delays for data from ABCStar
  uint32_t val_delay2 = 0xaaaaaaaa;
  uint32_t addr_delay2 = updateHCCRegister(HCCStarRegister::Delay2, val_delay2, cfg);
  sendCommand(star.write_hcc_register(addr_delay2, val_delay2), hwCtrl);

  uint32_t val_delay3 = 0x00000aaa;
  uint32_t addr_delay3 = updateHCCRegister(HCCStarRegister::Delay3, val_delay3, cfg);
  sendCommand(star.write_hcc_register(addr_delay3, val_delay3), hwCtrl);

  // Register DRV1: enable driver and currents
  uint32_t val_drv1 = 0x0fffffff;
  uint32_t addr_drv1 = updateHCCRegister(HCCStarRegister::DRV1, val_drv1, cfg);
  sendCommand(star.write_hcc_register(addr_drv1, val_drv1), hwCtrl);

  // Register ICenable: enable input channels
  uint32_t val_icen = ic_enables;
  uint32_t addr_icen = updateHCCRegister(HCCStarRegister::ICenable, val_icen, cfg);
  sendCommand(star.write_hcc_register(addr_icen, val_icen), hwCtrl);

  // Set normal readout using OPmode/OPmodeC
  setOpMode(0, mode640, hwCtrl, cfg);

  if (reset) {
    // Register ExtRst/ExtRstC: external reset for ABCStars
    uint32_t val_extrst = 0x00000001;

    uint32_t addr_extrst = updateHCCRegister(HCCStarRegister::ExtRst, val_extrst, cfg);
    uint32_t addr_extrstc = updateHCCRegister(HCCStarRegister::ExtRstC, val_extrst, cfg);

    sendCommand(star.write_hcc_register(addr_extrst, val_extrst), hwCtrl);
    sendCommand(star.write_hcc_register(addr_extrstc, val_extrst), hwCtrl);
  }
}

void configureHCC_PacketTransp(HwController& hwCtrl, StarCfg& cfg, bool reset, unsigned icEnablesMask, bool mode640) {
  configureHCC(hwCtrl, cfg, reset, icEnablesMask, mode640);

  // Set to packet transparent mode
  logger->info("Set HCCs to Packet Transparent mode");

  setOpMode(2, mode640, hwCtrl, cfg);
}

void configureHCC_FullTransp(HwController& hwCtrl, StarCfg& cfg, bool reset, unsigned inChn, unsigned icEnablesMask, bool mode640) {
  configureHCC(hwCtrl, cfg, reset, icEnablesMask, mode640);

  // Select the input channel: IC_transSelect
  // Register ICenable
  inChn = inChn & 0xf;
  if(((1<<inChn) & icEnablesMask) == 0) {
    logger->error("Selected IC {} is outside enables mask {}!", inChn, icEnablesMask);
  }
  unsigned val_icen = (inChn << 16) + (1 << inChn);
  uint32_t addr_icen = updateHCCRegister(HCCStarRegister::ICenable, val_icen, cfg);
  sendCommand(star.write_hcc_register(addr_icen, val_icen), hwCtrl);

  // Set to full transparent mode
  logger->info("Set HCCs to Full Transparent mode");

  setOpMode(3, mode640, hwCtrl, cfg);
}

void configureABC(HwController& hwCtrl, StarCfg& cfg, bool reset) {

  if (reset) {
    logger->info("Sending ABCStar register reset commands");
    sendCommand(LCB::fast_command(LCB::ABC_REG_RESET, 0), hwCtrl);
    sendCommand(LCB::fast_command(LCB::ABC_SLOW_COMMAND_RESET, 0), hwCtrl);
  }

  logger->info("Broadcast ABCStar configurations");

  // Set RR mode to 1
  auto [addr_rr, val_rr] = updateABCSubRegister(ABCStarSubRegister::RRMODE, 1, cfg);
  sendCommand(star.write_abc_register(addr_rr, val_rr), hwCtrl);

  // Enable LP
  auto [addr_lp, val_lp] = updateABCSubRegister(ABCStarSubRegister::LP_ENABLE, 1, cfg);
  sendCommand(star.write_abc_register(addr_lp, val_lp), hwCtrl);

  // Enable PR
  auto [addr_pr, val_pr] = updateABCSubRegister(ABCStarSubRegister::PR_ENABLE, 1, cfg);
  sendCommand(star.write_abc_register(addr_pr, val_pr), hwCtrl);

  // Set some mask registers to some nonzero value
  // MaskInput3
  logger->debug(" Set MaskInput3 to 0xfffe0000");
  uint32_t val_mask3 = 0xfffe0000;
  uint32_t addr_mask3 = updateABCRegister(ABCStarRegister::MaskInput3, val_mask3, cfg);
  sendCommand(star.write_abc_register(addr_mask3, val_mask3), hwCtrl);

  // MaskInput7
  logger->debug(" Set MaskInput7 to 0xff000000");
  uint32_t val_mask7 = 0xff000000;
  uint32_t addr_mask7 = updateABCRegister(ABCStarRegister::MaskInput7, val_mask7, cfg);
  sendCommand(star.write_abc_register(addr_mask7, val_mask7), hwCtrl);
}

// Test steps
bool checkHCCHPRs(HwController& hwCtrl,
                  const std::vector<uint32_t>& rxChannels,
                  bool reset, unsigned timeout_ms)
{
  if (reset) {
    sendCommand( LCB::fast_command(LCB::HCC_REG_RESET, 0), hwCtrl );
  }

  bool hprOK = false;

  for (auto rx : rxChannels) {
    logger->info("Reading HPR packets from Rx channel {}", rx);

    // Should not be necessary except for the emulator, but toggle the TestHPR
    // bit anyway in case HPR was stopped previously
    sendCommand(star.write_hcc_register((int)HCCStarRegister::Pulse, 0x2), hwCtrl);

    std::function<bool(RawData&)> filter_hpr = [rx](RawData& d) {
      return isPacketType(d, TYP_HCC_HPR) and isFromChannel(d, rx);
    };

    auto data = readData(hwCtrl, filter_hpr, timeout_ms);
    if (data) {
      logger->info(" Received an HPR packet from HCCStar on Rx channel {}", rx);
      // print
      StarChipPacket packet;
      if (packetFromRawData(packet, *data)) {
        logger->error("Packet parse failed");
      } else {
        std::stringstream os;
        packet.print_more(os);
        std::string str = os.str();
        str.erase(str.end()-1); // strip the extra \n
        logger->debug(" Received HPR packet: {}", str);

        //bool r3l1_locked = packet.value & (1<<0);
        //bool pll_locked = packet.value & (1<<5);
        bool lcb_locked = packet.value & (1<<1);
        if (lcb_locked) {
          logger->debug(" LCB locked");
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

bool probeHCCs(
  HwController& hwCtrl,
  std::vector<Hybrid>& HCCs,
  const std::vector<uint32_t>& txChannels,
  const std::vector<uint32_t>& rxChannels,
  bool setID,
  unsigned timeout_ms)
{
  HCCs.clear();
  uint32_t nHCC = 0;

  for (auto tx : txChannels) {
    bool hasHCCTx = false;

    logger->debug("Broadcast register commands to probe HCC via Tx channel {}", tx);
    hwCtrl.disableCmd();
    hwCtrl.setCmdEnable(tx);

    // Toggle bit 2 in register Pulse to load serial number into register Addressing
    sendCommand(star.write_hcc_register((int)HCCStarRegister::Pulse, 0x4), hwCtrl);

    // Scan through the Rx channels and look for response
    for (auto rx : rxChannels) {
      // Read the register Addressing
      sendCommand(star.read_hcc_register((int)HCCStarRegister::Addressing), hwCtrl);

      auto data = readData(
        hwCtrl,
        [rx](RawData& d) {
          return isPacketType(d, TYP_HCC_RR) and isFromChannel(d, rx);
        },
        timeout_ms
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
        logger_id->trace("HCCStar @ Tx = {} Rx = {}: ID = 0x{:x} eFuse ID = 0x{:06x}", tx, rx, hccID, fuseID);
        hasHCCTx = true;

        if (setID) {
          // Set HCC ID to nHCC
          uint32_t address = (nHCC << 28) | (fuseID & 0x00ffffff);
          sendCommand(star.write_hcc_register((int)HCCStarRegister::Addressing, address), hwCtrl);
          logger->info(" Set its ID to 0x{:x}", nHCC);
          hccID = nHCC;
        }

        // Define HCC, but still use broadcast for ABCs
        Hybrid h{tx, rx, hccID, BROADCAST_ABCS};
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
    return false;
  }

  // Enable only the Tx and Rx channels that are connected to the HCCs
  enableConnectedChannels(hwCtrl, HCCs);

  return true;
}

bool readMoreHCCRegisters(HwController& hwCtrl, unsigned timeout_ms)
{
  unsigned read_count = 0;

  // Loop over some register addresses
  // Not all, but relevant for low level diagnostics
  for (auto ra: std::vector<uint8_t>{3, 4, 5, 6, 7, 8, 9, 15, 17, 32, 33, 34, 35, 38, 39, 40, 41, 43}) {
    // Send register read command
    logger->debug("Broadcast read HCC Reg 0x{:02x} ({:12})",
                  ra, HccNames::regToString((HCCStarRegister)ra));
    sendCommand(star.read_hcc_register((int)ra), hwCtrl);

    auto rdc_rr = readAllData
      (
       // auto data = readData(
       hwCtrl,
       [](RawData& d) { return isPacketType(d, TYP_HCC_RR); },
       timeout_ms
       );

    for (unsigned c = 0; c < rdc_rr.size(); c++) {
      RawDataPtr data = rdc_rr.data[c];

      auto rx = data->getAdr();

      StarChipPacket packet;
      if (packetFromRawData(packet, *data)) {
        logger->error("Packet parse failed");
      } else {
        read_count ++;
        uint32_t value = packet.value;
        uint32_t addr = packet.address;

        std::string name = HccNames::regToString((HCCStarRegister)addr);

        logger->info(" HCCStar Reg 0x{:02x} ({:12}) @ RX {}: 0x{:08x}",
                     addr, name, rx, value);
      }
    } // Loop over data
  } // Loop over registers

  return read_count > 0;
}

bool checkABCHPRs(HwController& hwCtrl, StarCfg& cfg, std::vector<Hybrid>& hccStars, bool reset, unsigned timeout_ms) {
  bool receivedABCHPR = false;
  bool hprGood = true;

  if (reset) {
    sendCommand(LCB::fast_command(LCB::ABC_REG_RESET, 0), hwCtrl);
  }

  for (auto& hcc : hccStars) {
    logger->info("Reading HPR packets from ABCStars on HCCStar {}", hcc.hcc_id);

    // Enable the tx and rx channel
    hwCtrl.setCmdEnable(hcc.tx);
    hwCtrl.setRxEnable(hcc.rx);

    // Toggle the TestHPR bit in case of the emulator or HPR was previously stopped
    auto [addr_sc, val_sc] = updateABCSubRegister(ABCStarSubRegister::TESTHPR, 1, cfg);
    sendCommand(star.write_abc_register(addr_sc, val_sc), hwCtrl);

    std::function<bool(RawData&)> filter_abchpr = [&hcc](RawData& d) {
      return isPacketType(d, TYP_ABC_HPR) and isFromChannel(d, hcc.rx);
    };

    auto rdc = readAllData(hwCtrl, filter_abchpr, timeout_ms);

    for (unsigned c = 0; c < rdc.size(); c++) {
      RawDataPtr d = rdc.data[c];
      StarChipPacket packet;

      if ( packetFromRawData(packet, *d) ) {
        logger->error("Packet parse failed");
        continue;
      }

      logger->trace(" Received an HPR packet from ABCStar");
      receivedABCHPR = true;

      // check the input channel
      uint32_t abc_chn = packet.channel_abc;
      // get the chipID from the top four bits of the 16-bit status word
      uint32_t abcid = (packet.abc_status >> 12) & 0xf;

      if ( hcc.abcs.find(abc_chn) != hcc.abcs.end() ) {
        // already reported the HPR on this channel. skip.
        continue;
      }

      if(hcc.abcs == BROADCAST_ABCS) {
        // Remove broadcast now we know better
        hcc.abcs.clear();
      }

      // HPR from a new channel
      hcc.abcs[abc_chn] = abcid;
      logger->info(" Received an HPR packet from the ABCStar on channel {} with chipID {}", abc_chn, abcid);

      // print the HPR packet
      std::stringstream os;
      packet.print_more(os);
      std::string str = os.str();
      str.erase(str.end()-1); // strip the extra \n
      logger->debug(" Received HPR packet: {}", str);

      // check flags in HPR
      bool lcb_scmd_err = packet.value & (1<<15);
      if (lcb_scmd_err) logger->warn(" LCB slow command error");

      bool lcb_errcnt_ovfl = packet.value & (1<<14);
      if (lcb_errcnt_ovfl) logger->warn(" LCB error count overflow");

      bool lcb_decode_err = packet.value & (1<<13);
      if (lcb_decode_err) logger->warn(" LCB decode error");

      bool lcb_locked = packet.value & (1<<12);
      if(lcb_locked) {
        logger->debug(" LCB locked");
      } else {
        logger->error(" LCB NOT locked");
        hprGood = false;
      }

    } // end of data container loop

  } // end of HCC loop

  if (not receivedABCHPR) {
    logger->error("No HPR packet from ABCStar received from any HCCStars");
  }

  // Restore channel enable flags
  enableConnectedChannels(hwCtrl, hccStars);

  return receivedABCHPR and hprGood;
}

bool probeABCs(HwController& hwCtrl, StarCfg& cfg, std::vector<Hybrid>& hccStars, unsigned icEnablesMask, unsigned timeout_ms) {
  bool hasABCStar = false;

  for (auto& hcc : hccStars) {
    logger->info("Probing ABCStars on HCCStar {}", hcc.hcc_id);
    logger_id->trace("HCCStar {}", hcc.hcc_id);

    // Enable the tx and rx channel
    hwCtrl.setCmdEnable(hcc.tx);
    hwCtrl.setRxEnable(hcc.rx);

    unsigned activeInChannels = 0;

    // Read ABC efuse IDs
    // Toggle the EFUSEL bit first to load the 24b efuse bits to the register STAT2
    auto [addr_ef, val_ef] = updateABCSubRegister(ABCStarSubRegister::EFUSEL, 1, cfg);
    sendCommand(star.write_abc_register(addr_ef, val_ef), hwCtrl);

    // Broadcast the command to read the STAT2 registers of all ABCs associated with this HCC
    int stat2 = (int)ABCStarRegister::STAT2;
    sendCommand(star.read_abc_register(stat2), hwCtrl);

    // Read all the RR packets
    auto rdc_rr = readAllData(
      hwCtrl,
      [&](RawData& d) {return isPacketType(d, TYP_ABC_RR) and isFromChannel(d, hcc.rx);},
      timeout_ms
    );

    for (unsigned c = 0; c < rdc_rr.size(); c++) {
      RawDataPtr d = rdc_rr.data[c];
      StarChipPacket packet;
      if ( packetFromRawData(packet, *d) ) {
        logger->error("Packet parse failed");
        continue;
      }

      logger->trace(" Received a Register Read packet from ABCStar");

      // check the input channel
      uint32_t abc_chn = packet.channel_abc;
      // get the chipID from the top four bits of the 16-bit status word
      uint32_t abcid = (packet.abc_status >> 12) & 0xf;

      // Add abcid to hcc.abcs in case checkABCHPRs has not been called previously
      if ( hcc.abcs.find(abc_chn) == hcc.abcs.end() ) {
        hcc.abcs[abc_chn] = abcid;
      }

      // Update the active input channel mask
      unsigned newInMask = (1 << abc_chn);
      if(newInMask & icEnablesMask) {
        activeInChannels |= newInMask;

        // efuseID
        uint32_t abcFuseID = packet.value & 0x00ffffff; // lowest 24 bits
        //uint32_t abcStarVer = (packet.value & 0xff000000) >> 28; // top 8 bits
        logger->info(" Found ABCStar on HCCStar {}: Input channel = {} ABC ID = {} eFuse = 0x{:06x}", hcc.hcc_id, abc_chn, abcid, abcFuseID);
        logger_id->trace(" ABCStar: Input channel = {} ABC ID = {} eFuse = 0x{:06x}", abc_chn, abcid, abcFuseID);
      } else {
        logger->debug(" Ignoring ABCStar on HCCStar {}: Input channel = {}, outside IC mask {:03x}", hcc.hcc_id, abc_chn, icEnablesMask);
      }
    } // end of data container loop

    if (activeInChannels) {
      hasABCStar = true;
      // Update HCC register ICenable
      logger->debug("Set register ICenable on HCCStar {} to 0x{:08x}", hcc.hcc_id, activeInChannels);
      uint32_t addr_en = updateHCCRegister(HCCStarRegister::ICenable, activeInChannels, cfg);
      sendCommand(star.write_hcc_register(addr_en, activeInChannels, hcc.hcc_id), hwCtrl);
    } else {
      logger->error(" Found no ABCStar from HCCStar {}", hcc.hcc_id);
    }
  } // end of HCC loop

  if (not hasABCStar) {
    logger->error("No ABCStar from any HCCStar");
  }

  // Restore channel enable flags
  enableConnectedChannels(hwCtrl, hccStars);

  return hasABCStar;
}

bool readMoreABCRegisters(HwController& hwCtrl, unsigned timeout_ms) {
  unsigned read_count = 0;

  // Loop over a set of interesting ABC registers
  // Most cfg and status + 1 of each mask
  for (auto ra : std::vector<uint8_t>{1, 2, 3, 16, 32, 33, 34, 48, 49, 50, 51, 63, 64, 96, 104, 128}) {
    // Broadcast the read register command
    logger->debug("Broadcast read ABC Reg 0x{:02x} ({:12})",
                  ra, AbcNames::regToString((ABCStarRegister)ra));
    sendCommand(star.read_abc_register(ra), hwCtrl);

    // Read all the RR packets
    auto rdc_rr = readAllData
      (
       hwCtrl,
       [&](RawData& d) {return isPacketType(d, TYP_ABC_RR);},
       timeout_ms
       );

    for (unsigned c = 0; c < rdc_rr.size(); c++) {
      RawDataPtr d = rdc_rr.data[c];

      auto rx = d->getAdr();

      StarChipPacket packet;
      if ( packetFromRawData(packet, *d) ) {
        logger->error("Packet parse failed");
        continue;
      }

      read_count ++;

      // check the input channel
      uint32_t abc_chn = packet.channel_abc;
      // get the chipID from the top four bits of the 16-bit status word
      uint32_t abcid = (packet.abc_status >> 12) & 0xf;

      uint32_t value = packet.value;
      uint32_t addr = packet.address;

      std::string name = AbcNames::regToString((ABCStarRegister)addr);

      logger->info(" ABCStar Reg 0x{:02x} ({:12}) @ RX {}: IC {} ABC ID {}: 0x{:08x} 0x{:04x}",
		   addr, name, rx, abc_chn, abcid, value, packet.abc_status);
    } // end of data container loop
  } // Loop over registers

  return read_count > 0;
}

bool testRegisterReadWrite(HwController& hwCtrl, unsigned timeout_ms, uint32_t regAddr, uint32_t write_value, uint32_t rx, int hccId, int abcId=-1) {
  bool isHCC = abcId < 0;

  ////
  // Test register read
  unsigned int reg_read_value = 0xfffffff;

  std::string reg_str;
  PacketType ptype;
  if (isHCC) { // HCC register
    reg_str = "register "+std::to_string(regAddr)+" on HCCStar "+std::to_string(hccId);
    ptype = TYP_HCC_RR;

    logger->debug(" Reading "+reg_str);

    // Send register read command
    sendCommand(star.read_hcc_register(regAddr, hccId), hwCtrl);

  } else { // ABC register
    reg_str = "register "+std::to_string(regAddr)+" on ABCStar "+std::to_string(abcId)+" of HCCStar "+std::to_string(hccId);
    ptype = TYP_ABC_RR;

    logger->debug(" Reading "+reg_str);

    // Send register read command
    sendCommand(star.read_abc_register(regAddr, hccId, abcId), hwCtrl);
  }

  // Read data
  auto data = readData(
    hwCtrl,
    [&](RawData& d) {return isPacketType(d, ptype) and isFromChannel(d, rx);},
    timeout_ms
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

  logger->debug(" Writing value 0x{:08x} to {}", write_value, reg_str);
  if (isHCC) {
    sendCommand(star.write_hcc_register(regAddr, write_value, hccId), hwCtrl);
    logger->debug(" Reading "+reg_str);
    sendCommand(star.read_hcc_register(regAddr, hccId), hwCtrl);
  } else {
    sendCommand(star.write_abc_register(regAddr, write_value, hccId, abcId), hwCtrl);
    logger->debug(" Reading "+reg_str);
    sendCommand(star.read_abc_register(regAddr, hccId, abcId), hwCtrl);
  }

  bool regAccessGood = true;

  // Read data
  auto wdata = readData(
    hwCtrl,
    [&](RawData& d) {return isPacketType(d, ptype) and isFromChannel(d, rx);},
    timeout_ms
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
      regAccessGood = false;
    }
  } else {
    logger->error("Failed to read data");
    regAccessGood = false;
  }

  // Set the register back to its value before the test
  logger->debug(" Restore register {} to value 0x{:08x}", reg_str, reg_read_value);
  if (isHCC) {
    sendCommand(star.write_hcc_register(regAddr, reg_read_value, hccId), hwCtrl);
  } else {
    sendCommand(star.write_abc_register(regAddr, reg_read_value, hccId, abcId), hwCtrl);
  }

  return regAccessGood;
}

bool testHCCRegisterAccess(HwController& hwCtrl, const std::vector<Hybrid>& hccStars, unsigned timeout_ms) {
  logger->info("Test HCCStar register read & write");

  bool success = not hccStars.empty();

  if(hccStars.empty()) {
    logger->debug("No configured Hybrids for testHCCRegisterAccess");
  }

  for (const auto& hcc : hccStars) {
    // Register ErrCfg
    success &= testRegisterReadWrite(hwCtrl, timeout_ms, (uint32_t)HCCStarRegister::ErrCfg, 0xdeadbeef, hcc.rx, hcc.hcc_id);
  }

  return success;
}

bool testABCRegisterAccess(HwController& hwCtrl, StarCfg& cfg, const std::vector<Hybrid>& hccStars, unsigned timeout_ms) {
  logger->info("Test ABCStar register read & write");

  // Set RR mode to 1
  auto [addr_rr, val_rr] = updateABCSubRegister(ABCStarSubRegister::RRMODE, 1, cfg);
  sendCommand(star.write_abc_register(addr_rr, val_rr), hwCtrl);

  bool success = not hccStars.empty();

  if(hccStars.empty()) {
    logger->debug("No configured Hybrids for testABCRegisterAccess");
  }

  for (const auto& hcc : hccStars) {
    if (hcc.abcs.empty())
      success = false;

    for (const auto& abc : hcc.abcs) {
      // Register MaskInput0
      uint32_t mr = (uint32_t)ABCStarRegister::MaskInput0;
      success &= testRegisterReadWrite(hwCtrl, timeout_ms, mr, 0xabadcafe, hcc.rx, hcc.hcc_id, abc.second);
    }
  }

  return success;
}

bool testHitCounts(HwController& hwCtrl, StarCfg& cfg, unsigned timeout_ms) {
  logger->info("Test ABCStar hit counters");

  // Enable hit counters
  logger->debug(" Enable hit counters and set TM to 1 (static test mode)");
  // TM: 1
  auto [addr_tm, val_tm] = updateABCSubRegister(ABCStarSubRegister::TM, 1, cfg);
  sendCommand(star.write_abc_register(addr_tm, val_tm), hwCtrl);
  // RR mode: 1
  auto [addr_rr, val_rr] = updateABCSubRegister(ABCStarSubRegister::RRMODE, 1, cfg);
  sendCommand(star.write_abc_register(addr_rr, val_rr), hwCtrl);
  // EnCount: 1
  auto [addr_cnt, val_cnt] = updateABCSubRegister(ABCStarSubRegister::ENCOUNT, 1, cfg);
  sendCommand(star.write_abc_register(addr_cnt, val_cnt), hwCtrl);
  // LP_ENABLE: 0
  auto [addr_lp, val_lp] = updateABCSubRegister(ABCStarSubRegister::LP_ENABLE, 0, cfg);
  sendCommand(star.write_abc_register(addr_lp, val_lp), hwCtrl);
  // PR_ENABLE: 0
  auto [addr_pr, val_pr] = updateABCSubRegister(ABCStarSubRegister::PR_ENABLE, 0, cfg);
  sendCommand(star.write_abc_register(addr_pr, val_pr), hwCtrl);

  // Reset and start ABCStar hit counters
  logger->debug(" Reset hit counters");
  sendCommand(LCB::fast_command(LCB::ABC_HIT_COUNT_RESET, 0), hwCtrl);
  logger->debug(" Start hit counters");
  sendCommand(LCB::fast_command(LCB::ABC_HIT_COUNT_START, 0), hwCtrl);

  // Send BC reset
  logger->debug(" Send BC reset");
  sendCommand(LCB::lonely_bcr(), hwCtrl);

  // Send triggers
  logger->debug(" Send a trigger 10 times");
  for (int i=0; i<10; i++) {
    sendCommand(LCB::l0a_mask(1,42+i,false), hwCtrl);
  }

  // Stop hit counters
  logger->debug(" Stop hit counters");
  sendCommand(LCB::fast_command(LCB::ABC_HIT_COUNT_STOP, 0), hwCtrl);

  // Read a hit counter register that corresponds to the masked strips
  logger->debug(" Read register HitCountREG63");
  int cr63 = (int)ABCStarRegister::HitCountREG63;
  sendCommand(star.read_abc_register(cr63), hwCtrl);

  auto data = readData(
    hwCtrl,
    [](RawData& d) {return isPacketType(d, TYP_ABC_RR);},
    timeout_ms
    );

  hwCtrl.flushBuffer();

  if (data) {
    StarChipPacket packet;
    packetFromRawData(packet, *data);

    if (logger->should_log(spdlog::level::debug)) {
      // print packet
      std::stringstream os;
      packet.print_more(os);
      std::string str = os.str();
      str.erase(str.end()-1); // strip the extra \n
      logger->debug(" Received register read packet: {}", str);
    }

    // expected hit counts
    unsigned hitcounts_exp = 0x0a0a0a0a;
    if (packet.value == hitcounts_exp) {
      logger->info("Hit counts: OK");
    } else {
      logger->error("Hit counts do not agree with what is expected: 0x{:08x} (read) 0x{:08x} (expect)", packet.value, hitcounts_exp);
      return false;
    }
  } else {
    logger->error("Fail to read the hit counter register");
    return false;
  }

  return true;
}

bool testDataPacketsStatic(HwController& hwCtrl, StarCfg& cfg, unsigned timeout_ms) {
  logger->info("Read ABCStar data packets in static mode");

  // Static test mode first
  logger->debug(" Set TM to 1 and enable LP and PR");
  // TM: 1
  auto [addr_tm, val_tm] = updateABCSubRegister(ABCStarSubRegister::TM, 1, cfg);
  sendCommand(star.write_abc_register(addr_tm, val_tm), hwCtrl);
  // RR mode: 1
  auto [addr_rr, val_rr] = updateABCSubRegister(ABCStarSubRegister::RRMODE, 1, cfg);
  sendCommand(star.write_abc_register(addr_rr, val_rr), hwCtrl);
  // EnCount: 0
  auto [addr_cnt, val_cnt] = updateABCSubRegister(ABCStarSubRegister::ENCOUNT, 0, cfg);
  sendCommand(star.write_abc_register(addr_cnt, val_cnt), hwCtrl);
  // LP_ENABLE: 1
  auto [addr_lp, val_lp] = updateABCSubRegister(ABCStarSubRegister::LP_ENABLE, 1, cfg);
  sendCommand(star.write_abc_register(addr_lp, val_lp), hwCtrl);
  // PR_ENABLE: 1
  auto [addr_pr, val_pr] = updateABCSubRegister(ABCStarSubRegister::PR_ENABLE, 1, cfg);
  sendCommand(star.write_abc_register(addr_pr, val_pr), hwCtrl);

  // Enable PR & LP
  sendCommand(LCB::fast_command(LCB::HCC_START_PRLP, 0), hwCtrl);

  // BC Reset
  sendCommand(LCB::lonely_bcr(), hwCtrl);

  // Send a trigger
  sendCommand(LCB::l0a_mask(1, 233, false), hwCtrl);

  // Read the data packets
  auto rdc = readAllData(
    hwCtrl, [](RawData& d) {return isPacketType(d, TYP_LP);},
    timeout_ms
    );

  hwCtrl.flushBuffer();

  for (unsigned c = 0; c < rdc.size(); c++) {
    RawDataPtr d = rdc.data[c];
    reportData(*d);
  }

  if (rdc.size() > 0) {
    logger->info("Read data packets in static test mode: Ok");
  } else {
    logger->error("Fail to read data packets in static test mode");
    return false;
  }

  return true;
}

bool testDataPacketsPulse(HwController& hwCtrl, StarCfg& cfg, unsigned timeout_ms) {
  logger->info("Read ABCStar data packets in test pulse mode");

  // Test pulse mode
  logger->debug(" Set TM to 2 and enable test pulse");
  // TM: 2
  auto [addr_tm, val_tm] = updateABCSubRegister(ABCStarSubRegister::TM, 2, cfg);
  sendCommand(star.write_abc_register(addr_tm, val_tm), hwCtrl);
  // RR mode: 1
  auto [addr_rr, val_rr] = updateABCSubRegister(ABCStarSubRegister::RRMODE, 1, cfg);
  sendCommand(star.write_abc_register(addr_rr, val_rr), hwCtrl);
  // EnCount: 0
  auto [addr_cnt, val_cnt] = updateABCSubRegister(ABCStarSubRegister::ENCOUNT, 0, cfg);
  sendCommand(star.write_abc_register(addr_cnt, val_cnt), hwCtrl);
  // LP_ENABLE: 1
  auto [addr_lp, val_lp] = updateABCSubRegister(ABCStarSubRegister::LP_ENABLE, 1, cfg);
  sendCommand(star.write_abc_register(addr_lp, val_lp), hwCtrl);
  // PR_ENABLE: 1
  auto [addr_pr, val_pr] = updateABCSubRegister(ABCStarSubRegister::PR_ENABLE, 1, cfg);
  sendCommand(star.write_abc_register(addr_pr, val_pr), hwCtrl);
  // TEST_PULSE_ENABLE: 1
  auto [addr_tp, val_tp] = updateABCSubRegister(ABCStarSubRegister::TEST_PULSE_ENABLE, 1, cfg);
  sendCommand(star.write_abc_register(addr_tp, val_tp), hwCtrl);

  // Set the L0 pipeline latency to a smaller value: 15
  uint32_t abc_latency = 15;
  logger->debug(" Set L0 latency to {}", abc_latency);
  auto [addr_lat, val_lat] = updateABCSubRegister(ABCStarSubRegister::LATENCY, abc_latency, cfg);
  sendCommand(star.write_abc_register(addr_lat, val_lat), hwCtrl);

  // Set BCIDrstDelay of the HCC so we won't get BCID errors
  // L0 latency - 2 for ABCStar v0; L0 latency - 6 for ABCStar v1
  // Get ABC version from StarCfg?
  uint32_t bcdelay = abc_latency - 2;
  auto [addr_delay, val_delay] = updateHCCSubRegister(HCCStarSubRegister::BCIDRSTDELAY, bcdelay, cfg);
  sendCommand(star.write_hcc_register(addr_delay, val_delay), hwCtrl);

  // BC reset
  logger->debug(" Send BC reset");
  sendCommand(LCB::lonely_bcr(), hwCtrl);

  // Send a digital pulse followed by a trigger
  logger->debug(" Send a digital pulse followed by a trigger");
  std::array<uint16_t, 9> cmd = {
    LCB::IDLE, LCB::IDLE,
    LCB::fast_command(LCB::ABC_DIGITAL_PULSE, 0), LCB::IDLE,
    LCB::IDLE, LCB::IDLE,
    LCB::l0a_mask(1, 43, false),
    LCB::IDLE, LCB::IDLE
  };
  sendCommand(cmd, hwCtrl);

  // Read the data packets
  auto rdc = readAllData(
    hwCtrl, [](RawData& d) {return isPacketType(d, TYP_LP);},
    timeout_ms
    );

  hwCtrl.flushBuffer();

  for (unsigned c = 0; c < rdc.size(); c++) {
    RawDataPtr d = rdc.data[c];
    reportData(*d);
  }

  if (rdc.size() > 0) {
    logger->info("Read data packets in test pulse mode: Ok");
  } else {
    logger->error("Fail to read data packets in test pulse mode");
    return false;
  }

  return true;
}

bool readABCRegisters(HwController& hwCtrl, unsigned timeout_ms) {

  bool success = false;

  hwCtrl.flushBuffer();

  // Read an ABCStar HPR
  logger->info("Reading an HPR packet from ABCStar");
  auto data_abchpr = readData(
    hwCtrl,
    [](RawData& d) {return isPacketType(d, TYP_ABC_HPR);},
    timeout_ms
    );
  if (data_abchpr) {
    logger->info("Received an ABCStar HPR packet.");
    reportData(*data_abchpr);
    success = true;
  } else {
    logger->error("No ABCStar HPR packet received.");
    success = false;
  }

  // Read ABCStar register MaskInput3
  logger->info("Reading ABCStar register MaskInput3");
  int mr3 = (int)ABCStarRegister::MaskInput3;
  sendCommand(star.read_abc_register(mr3), hwCtrl);
  auto data_abcrr = readData(
    hwCtrl,
    [](RawData& d) {return isPacketType(d, TYP_ABC_RR);},
    timeout_ms
    );
  if (data_abcrr) {
    logger->info("Received an ABCStar RR packet.");
    reportData(*data_abcrr);
    success &= true;
  } else {
    logger->error("No ABCStar RR packet received.");
    success = false;
  }

  return success;
}

bool diagnosticsReport(HwController &hwCtrl, unsigned timeout_ms) {
  // try reading everything for 1 seconds
  auto rdc = readAllData(
        hwCtrl,
        [](RawData& d) {return true;}, // no filter on data packet type
        timeout_ms
        );

  for (unsigned c = 0; c < rdc.size(); c++) {
    RawDataPtr d = rdc.data[c];
    reportData(*d);
  }

  logger->info("Complete report on {} packets", rdc.size());

  return true;
}

} // end of unnamed namespace

//////////
int main(int argc, char *argv[]) {
    std::string controller;
    std::string controllerType;

    TestData testData;

    std::string testSequence("Full");
    std::string chipVersion("Star");

    // logger config path
    std::string logCfgPath = "";
    bool doReport = false;
    bool doCrossCheck = false;

    const struct option long_options[] =
      {
        {"help", no_argument, nullptr, 'h'},
        {"timeout", no_argument, nullptr, 'w'},
        {nullptr, 0, nullptr, 0}};

    int c;
    while ((c = getopt_long(argc, argv, "3hvTi:l:r:t:dRs:c:w:V:", long_options, nullptr)) != -1) {
      switch(c) {
      case 'h':
        printHelp();
        return 0;
      case '3':
        testData.mode640 = false;
        break;
      case 'i':
        try {
          // Allow hex
          size_t pos = 0;
          testData.icEnablesMask = std::stoul(optarg, &pos, 0);
          if(pos != strlen(optarg)) {
            spdlog::error("Failed to parse ic enables mask: {}", optarg);
            return 1;
          }
        } catch(std::exception &e) {
          // stoul throws if no digits at all 
          spdlog::error("Failed to parse ic enables mask: {}", optarg);
          return 1;
        }
        break;
      case 'l':
        logCfgPath = std::string(optarg);
        break;
      case 'r':
        testData.rxChannels.clear();
        optind -= 1;
        for (; optind < argc && *argv[optind] != '-'; optind += 1) {
          try {
            // Try parsing as number and throw if not
            testData.rxChannels.push_back( std::stoi(argv[optind]) );
          } catch(std::exception &e) {
            break;
          }
        }
        break;
      case 't':
        testData.txChannels.clear();
        optind -= 1;
        for (; optind < argc && *argv[optind] != '-'; optind += 1) {
          try {
            // Try parsing as number and throw if not
            testData.txChannels.push_back( std::stoi(argv[optind]) );
          } catch(std::exception &e) {
            break;
          }
        }
        break;
      case 'w':
        try {
          testData.timeout_ms = std::stoul(optarg);
        } catch(std::exception &e) {
          spdlog::error("Failed to parse timeout: {}", optarg);
          return 1;
        }
        break;
      case 'd':
        testData.setHccId = true;
        break;
      case 'R':
        testData.doResets = true;
        break;
      case 'v':
        doReport = true;
        break;
      case 's':
        testSequence = std::string(optarg);
        break;
      case 'c':
        testData.inChannel = atoi(optarg);
        if (testData.inChannel > Star::MaxABCsPerHCC) {
          spdlog::error("Invalid HCC input channel: {}", testData.inChannel);
          return 1;
        }
        break;
      case 'T':
        doCrossCheck = true;
        break;
      case 'V':
        chipVersion = std::string(optarg);
        break;
      default:
        spdlog::critical("Error while parsing command line parameters!");
        return -1;
      }
    }

    if(doCrossCheck) {
      auto tests = testData.buildTests();

      if(!testData.crossCheck()) {
        return 1;
      } else {
        std::cout << "Sequence/test cross-check passed\n";
        return 0;
      }
    }

    // Configure logger
    if (logCfgPath.empty()) {
      json j; // Start empty
      std::string defaultLogPattern = "[%T:%e]%^[%=8l][%=15n]:%$ %v";
      j["pattern"] = defaultLogPattern;
      j["log_config"][0]["name"] = "all";
      j["log_config"][0]["level"] = "info";
      logging::setupLoggers(j);
    } else {
      try {
        auto j = ScanHelper::openJsonFile(logCfgPath);
        logging::setupLoggers(j);
      } catch (std::runtime_error &e) {
        spdlog::error("Opening logger config: {}", e.what());
        return 1;
      }
    }

    // Star chip configuration
    unsigned abc_version, hcc_version;
    if (chipVersion == "Star") { // for now
      hcc_version = 0;
      abc_version = 0;
    } else if (chipVersion == "Star_vH0A0") { // a.k.a. Prototype
      hcc_version = 0;
      abc_version = 0;
    } else if (chipVersion == "Star_vH0A1") { // a.k.a. PPA
      hcc_version = 0;
      abc_version = 1;
    } else if (chipVersion == "Star_vH1A1") { // a.k.a. PPB
      hcc_version = 1;
      abc_version = 1;
    } else {
      logger->error("Unknown Star chip version! Possible options are: Star, Star_vH0A0, Star_vH0A1, Star_vH1A1");
      return 1;
    }

    logger->debug("Run with{}reset, speed {}{}",
                  testData.doResets?" ":"-out ",
                  testData.mode640?"640":"320",
                  testData.setHccId?", and set HCC IDs":""
                  );

    // A global StarCfg with dummy chip configs
    testData.starCfg = std::make_unique<StarCfg>(abc_version, hcc_version);
    testData.starCfg->setHCCChipId(0xf);
    testData.starCfg->addABCchipID(0xf);

    // Controller
    if (optind != argc) {
      // First positional parameter (optind is first not parsed by getopt)
      controller = argv[optind];
    }

    std::unique_ptr<HwController> hwCtrl = nullptr;
    if(controller.empty()) {
      logger->error("No controller specified");
      return 1;
    } else {
      try {
        logger->info("Using controller from {}", controller);
        json ctrlCfg = ScanHelper::openJsonFile(controller);
        controllerType = ctrlCfg["ctrlCfg"]["type"];
        hwCtrl = StdDict::getHwController(controllerType);
        hwCtrl->loadConfig(ctrlCfg["ctrlCfg"]["cfg"]);
      } catch (std::runtime_error &e) {
        logger->error("Opening controller config: {}", e.what());
        return 1;
      }
    }

    if(!hwCtrl) {
      std::cout << "Failed to select valid HwController aborting\n";
      return 1;
    }

    hwCtrl->toggleTrigAbort();
    hwCtrl->setTrigEnable(0);

    // Enable Tx channels
    for(auto t: testData.txChannels) {
      logger->debug("Enable tx {}", t);
    }
    hwCtrl->setCmdEnable(testData.txChannels);

    // Enable Rx channels
    hwCtrl->disableRx();
    for(auto r: testData.rxChannels) {
      logger->debug("Enable rx {}", r);
    }
    hwCtrl->initRxChannels(testData.rxChannels);
    hwCtrl->setRxEnable(testData.rxChannels);

    // Tests
    bool success = true;

    auto tests = testData.buildTests();

    testData.validate();

    if(isupper(testSequence[0])) {
      if(sequenceMap.find(testSequence) != sequenceMap.end()) {
        logger->info("Running test sequence {}", testSequence);
        if(doReport) {
          testData.report();
        }
        for(auto &t: sequenceMap[testSequence]) {
          logger->info("Running test {}", t);
          success &= tests[t](*hwCtrl);
          if(doReport) {
            testData.report();
          }
        }
      } else {
        logger->error("Unknown test sequence: {}", testSequence);

        logger->info("Available preset test sequences:");

        for(auto &s: sequenceMap) {
          logger->info("  {}", s.first);
        }

        success = false;
      }
    } else {
      if(tests.find(testSequence) != tests.end()) {
        logger->info("Running test {}", testSequence);
        if(doReport) {
          testData.report();
        }
        success &= tests[testSequence](*hwCtrl);
        if(doReport) {
          testData.report();
        }
      } else {
        logger->error("Unknown test: {}", testSequence);

        logger->info("Available test presets:");

        for(auto &s: tests) {
          logger->info("  {}", s.first);
        }

        success = false;
      }
    }

    for(auto r: testData.rxChannels) {
      logger->debug("Disable Rx channels");
    }
    hwCtrl->disableRx();

    if (not success) {
      logger->error("Tests failed");
      return 1;
    }

    logger->info("Success!");
    return 0;
}

std::map<std::string, std::function<bool (HwController&)>> TestData::buildTests () {
  std::map<std::string, std::function<bool (HwController&)>>
      tests = {
      // Read HCCStar HPRs
      {"checkHCCHPRs", [&](auto &h) {return checkHCCHPRs(h, rxChannels, doResets, timeout_ms);}},
      // Probe HCCs
      {"probeHCCs", [&](auto &h) {return probeHCCs(h, hccStars, txChannels, rxChannels, setHccId, timeout_ms);}},
      // Read many HCC registers
      {"readMoreHCCRegisters", [&](auto &h) {return readMoreHCCRegisters(h, timeout_ms);}},
      // Test HCCStar register read and write
      {"testHCCRegister", [&](auto &h) {return testHCCRegisterAccess(h, hccStars, timeout_ms);}},

      // Configure HCCs to enable communications with ABCs
      {"configureHCC", [&](auto &h) {configureHCC(h, *starCfg, doResets, icEnablesMask, mode640); return true;}},
      {"configureHCCIfReset", [&](auto &h) {if(doResets) configureHCC(h, *starCfg, doResets, icEnablesMask, mode640); return true;}},
      // configure HCC into the Packet Transparent mode
      {"configureHCCForPacketTransp", [&](auto &h) {configureHCC_PacketTransp(h, *starCfg, doResets, icEnablesMask, mode640); return true; }},
      {"configureHCCForFullTransp", [&](auto &h) {configureHCC_FullTransp(h, *starCfg, doResets, inChannel, icEnablesMask, mode640); return true; }},

      // Probe ABCStars via reading ABCStar HPRs
      // Check ABCStar HPRs
      {"checkABCHPRs", [&](auto &h) {return checkABCHPRs(h, *starCfg, hccStars, doResets, timeout_ms);}},
      // Probe ABCStars on each HCCStar
      {"probeABCs", [&](auto &h) {return probeABCs(h, *starCfg, hccStars, icEnablesMask, timeout_ms);}},
      // Read many ABC registers
      {"readMoreABCRegisters", [&](auto &h) {return readMoreABCRegisters(h, timeout_ms);}},
      // Test ABCStar register read and write
      {"testABCRegister", [&](auto &h) {return testABCRegisterAccess(h, *starCfg, hccStars, timeout_ms);}},

      // Configure ABCs
      {"configureABC", [&](auto &h) {configureABC(h, *starCfg, doResets); return true;}},

      // Read ABC hit counters
      {"testHitCounts", [&](auto &h) {return testHitCounts(h, *starCfg, timeout_ms);}},

      // Read ABC data packets
      {"readABCRegisters", [&](auto &h) {return readABCRegisters(h, timeout_ms);}},

      // More involved tests, put FrontEnd in mode and check response
      {"testDataPacketsStatic", [&](auto &h) {return testDataPacketsStatic(h, *starCfg, timeout_ms);}},
      {"testDataPacketsPulse", [&](auto &h) {return testDataPacketsPulse(h, *starCfg, timeout_ms);}},

      {"diagnosticsReport", [&](auto &h) {return diagnosticsReport(h, timeout_ms);}},
    };

  return tests;
}

bool TestData::crossCheck() {
    // Cross-validation between tests and sequenceMap
    // Mostly here so we can be sure the list from printHelp is accurate
    std::set<std::string> allTestNames;
    for(auto &tt: buildTests()) {
      allTestNames.insert(tt.first);
    }

    std::set<std::string> allSequenceTestNames;
    for(auto &ss: sequenceMap) {
      for(auto &tn: ss.second) {
        allSequenceTestNames.insert(tn);
      }
    }

    if(allTestNames != allSequenceTestNames) {
      std::cout << "Internal error, list of tests does not match list of tests in sequences\n";
      for(auto &tt: allTestNames) {
        std::cout << " " << tt;
        if(allSequenceTestNames.find(tt) == allSequenceTestNames.end()) {
          std::cout << " (only in tests)\n";
        }
        std::cout << "\n";
      }
      for(auto &tt: allSequenceTestNames) {
        if(allTestNames.find(tt) != allTestNames.end()) {
          continue;
        }
        std::cout << " " << tt << " (only in sequence tests)\n";
      }
      return false;
    }

    return true;
}
