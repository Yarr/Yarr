#include <bitset>
#include <iostream>
#include <functional>
#include <tuple>
#include <set>

#include "AllHwControllers.h"
#include "StarCmd.h"
#include "StarCfg.h"
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
    uint32_t tx;
    uint32_t rx;
    uint32_t hcc_id;
    std::map<uint32_t,uint32_t> abcs; // key: channel; value: chipID
  };

void printHelp() {
  std::cout << "Usage: test_star HW_CONFIG [OPTIONS] ... \n";
  std::cout << "   Run Star FE tests with HardwareController configuration from HW_CONFIG\n";
  std::cout << " -h: Show this help.\n";
  std::cout << " -r <channel1> [<channel2> ...] : Rx channels to enable. Can take multiple arguments.\n";
  std::cout << " -t <channel1> [<channel2> ...] : Tx channels to enable. Can take multiple arguments.\n";
  std::cout << " -l <log_config> : Configure loggers.\n";
  std::cout << " -d : Modify HCCStar IDs when probing.\n";
  std::cout << " -R : Send reset commands.\n";
  std::cout << " -s <test_preset> : Type of test sequence to run. Possible options are: Full, Register, DataPacket, Probe, PacketTransp, FullTransp. Default: Full\n";
  std::cout << " -c <input channel> : HCC input channel. Only used if HCCs are set to full transparent mode.\n";
  std::cout << " -V <chip_version> : Versions of the HCCStar and ABCStar chips. Possible options are: Star, Star_vH0A0, Star_vH0A1, Star_vH1A1. Default: Star (equivalent to Star_vH0A0)\n";
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
std::tuple<uint32_t, uint32_t> updateHCCSubRegister(const std::string& subRegName, uint32_t value, StarCfg& cfg) {
  // chipIndex 0 for HCC
  int chipIndex = 0;
  cfg.setSubRegisterValue(chipIndex, subRegName, value);

  uint32_t addr = cfg.getSubRegisterParentAddr(chipIndex, subRegName);
  uint32_t newValue = cfg.getSubRegisterParentValue(chipIndex, subRegName);

  return std::make_tuple(addr, newValue);
}

// Assume the register configuration command is always broadcasted to all chips
std::tuple<uint32_t, uint32_t> updateABCSubRegister(const std::string& subRegName, uint32_t value, StarCfg& cfg) {

  cfg.eachAbc([&](auto &abc) {
      abc.setSubRegisterValue(subRegName, value);
    });

  // chipIndex 1 for the first ABC
  int chipIndex = 1;
  uint32_t addr = cfg.getSubRegisterParentAddr(chipIndex, subRegName);
  uint32_t newValue = cfg.getSubRegisterParentValue(chipIndex, subRegName);

  return std::make_tuple(addr, newValue);
}

// Update the register value in the chip config
// Return the register address in int
uint32_t updateHCCRegister(const std::string& regName, uint32_t value, StarCfg& cfg) {
  HCCStarRegister addr = HCCStarRegister::_from_string(regName.c_str());
  cfg.setHCCRegister(addr, value);
  return addr;
}

uint32_t updateABCRegister(const std::string& regName, uint32_t value, StarCfg& cfg) {
  ABCStarRegister addr = ABCStarRegister::_from_string(regName.c_str());
  cfg.eachAbc([&](auto &abc) {
      abc.setRegisterValue(addr, value);
    });

  return addr;
}

// Enable Tx and Rx channels that are connected to HCCs
void enableConnectedChannels(HwController& hwCtrl, std::vector<Hybrid>& hccStars) {
  // Turn all channels off first
  hwCtrl.disableCmd();
  hwCtrl.disableRx();

  if (hccStars.empty())
    return;

  std::set<uint32_t> txChns;
  std::set<uint32_t> rxChns;

  for (auto& hcc : hccStars) {
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

  packet.add_word(0x13C); //add SOP
  for(unsigned iw=0; iw<data.getSize(); iw++) {
    for(int i=0; i<4;i++){
      packet.add_word((data[iw]>>i*8)&0xFF);
    }
  }
  packet.add_word(0x1DC); //add EOP

  return packet.parse();
}

RawDataPtr readData(
  HwController& hwCtrl,
  std::function<bool(RawData&)> filter_cb,
  uint32_t timeout=1000)
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
      for (auto d : dataVec) {
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

  return std::move(data);
}

RawDataContainer readAllData(
  HwController& hwCtrl,
  std::function<bool(RawData&)> filter_cb,
  uint32_t timeout=2000)
{
  bool nodata = true;

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
void configureHCC(HwController& hwCtrl, StarCfg& cfg, bool reset) {
  // Configure HCCStars to enable communications with ABCStars
  if (reset) {
    logger->info("Sending HCCStar register reset command");
    sendCommand(LCB::fast_command(LCB::HCC_REG_RESET, 0), hwCtrl);
  }

  logger->info("Broadcast HCCStar configurations");

  // Register Delay1: delays for signals to ABCStars
  uint32_t val_delay1 = 0x02400000;
  uint32_t addr_delay1 = updateHCCRegister("Delay1", val_delay1, cfg);
  sendCommand(star.write_hcc_register(addr_delay1, val_delay1), hwCtrl);

  // Register Delay2, Delay3: delays for data from ABCStar
  uint32_t val_delay2 = 0x44444444;
  uint32_t addr_delay2 = updateHCCRegister("Delay2", val_delay2, cfg);
  sendCommand(star.write_hcc_register(addr_delay2, val_delay2), hwCtrl);

  uint32_t val_delay3 = 0x00000444;
  uint32_t addr_delay3 = updateHCCRegister("Delay3", val_delay3, cfg);
  sendCommand(star.write_hcc_register(addr_delay3, val_delay3), hwCtrl);

  // Register DRV1: enable driver and currents
  uint32_t val_drv1 = 0x0fffffff;
  uint32_t addr_drv1 = updateHCCRegister("DRV1", val_drv1, cfg);
  sendCommand(star.write_hcc_register(addr_drv1, val_drv1), hwCtrl);

  // Register ICenable: enable input channels
  uint32_t val_icen = 0x000007ff;
  uint32_t addr_icen = updateHCCRegister("ICenable", val_icen, cfg);
  sendCommand(star.write_hcc_register(addr_icen, val_icen), hwCtrl);

  if (reset) {
    // Register ExtRst/ExtRstC: external reset for ABCStars
    uint32_t val_extrst = 0x00000001;

    uint32_t addr_extrst = updateHCCRegister("ExtRst", val_extrst, cfg);
    uint32_t addr_extrstc = updateHCCRegister("ExtRstC", val_extrst, cfg);

    sendCommand(star.write_hcc_register(addr_extrst, val_extrst), hwCtrl);
    sendCommand(star.write_hcc_register(addr_extrstc, val_extrst), hwCtrl);
  }
}

void configureHCC_PacketTransp(HwController& hwCtrl, StarCfg& cfg, bool reset) {
  configureHCC(hwCtrl, cfg, reset);

  // Set to packet transparent mode
  logger->info("Set HCCs to Packet Transparent mode");
  // Register OPmode/OPmodeC
  uint32_t val_mode = 0x00020201;

  uint32_t addr_mode = updateHCCRegister("OPmode", val_mode, cfg);
  uint32_t addr_modec = updateHCCRegister("OPmodeC", val_mode, cfg);

  sendCommand(star.write_hcc_register(addr_mode, val_mode), hwCtrl);
  sendCommand(star.write_hcc_register(addr_modec, val_mode), hwCtrl);
}

void configureHCC_FullTransp(HwController& hwCtrl, StarCfg& cfg, bool reset, unsigned inChn) {
  configureHCC(hwCtrl, cfg, reset);

  // Select the input channel: IC_transSelect
  // Register ICenable
  inChn = inChn & 0xf;
  unsigned val_icen = (inChn << 16) + (1 << inChn);
  uint32_t addr_icen = updateHCCRegister("ICenable", val_icen, cfg);
  sendCommand(star.write_hcc_register(addr_icen, val_icen), hwCtrl);

  // Set to full transparent mode
  logger->info("Set HCCs to Full Transparent mode");
  // Register OPmode/OPmodeC
  uint32_t val_mode = 0x00020301;

  uint32_t addr_mode = updateHCCRegister("OPmode", val_mode, cfg);
  uint32_t addr_modec = updateHCCRegister("OPmodeC", val_mode, cfg);

  sendCommand(star.write_hcc_register(addr_mode, val_mode), hwCtrl);
  sendCommand(star.write_hcc_register(addr_modec, val_mode), hwCtrl);
}

void configureABC(HwController& hwCtrl, StarCfg& cfg, bool reset) {

  if (reset) {
    logger->info("Sending ABCStar register reset commands");
    sendCommand(LCB::fast_command(LCB::ABC_REG_RESET, 0), hwCtrl);
    sendCommand(LCB::fast_command(LCB::ABC_SLOW_COMMAND_RESET, 0), hwCtrl);
  }

  logger->info("Broadcast ABCStar configurations");

  // Set RR mode to 1
  auto [addr_rr, val_rr] = updateABCSubRegister("RRMODE", 1, cfg);
  sendCommand(star.write_abc_register(addr_rr, val_rr), hwCtrl);

  // Enable LP
  auto [addr_lp, val_lp] = updateABCSubRegister("LP_ENABLE", 1, cfg);
  sendCommand(star.write_abc_register(addr_lp, val_lp), hwCtrl);

  // Enable PR
  auto [addr_pr, val_pr] = updateABCSubRegister("PR_ENABLE", 1, cfg);
  sendCommand(star.write_abc_register(addr_pr, val_pr), hwCtrl);

  // Set some mask registers to some nonzero value
  // MaskInput3
  logger->debug(" Set MaskInput3 to 0xfffe0000");
  uint32_t val_mask3 = 0xfffe0000;
  uint32_t addr_mask3 = updateABCRegister("MaskInput3", val_mask3, cfg);
  sendCommand(star.write_abc_register(addr_mask3, val_mask3), hwCtrl);

  // MaskInput7
  logger->debug(" Set MaskInput7 to 0xff000000");
  uint32_t val_mask7 = 0xff000000;
  uint32_t addr_mask7 = updateABCRegister("MaskInput7", val_mask7, cfg);
  sendCommand(star.write_abc_register(addr_mask7, val_mask7), hwCtrl);
}

// Test steps
bool checkHPRs(HwController& hwCtrl,
               const std::vector<uint32_t>& rxChannels,
               bool reset)
{
  if (reset) {
    sendCommand( LCB::fast_command(LCB::HCC_REG_RESET, 0), hwCtrl );
  }

  uint32_t timeout = 1000; // milliseconds

  bool hprOK = false;

  for (auto rx : rxChannels) {
    logger->info("Reading HPR packets from Rx channel {}", rx);

    // Should not be necessary except for the emulator, but toggle the TestHPR
    // bit anyway in case HPR was stopped previously
    sendCommand(star.write_hcc_register(HCCStarRegister::Pulse, 0x2), hwCtrl);

    std::function<bool(RawData&)> filter_hpr = [rx](RawData& d) {
      return isPacketType(d, TYP_HCC_HPR) and isFromChannel(d, rx);
    };

    auto data = readData(hwCtrl, filter_hpr, timeout);
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
  bool setID)
{
  HCCs.clear();
  uint32_t nHCC = 0;

  for (auto tx : txChannels) {
    bool hasHCCTx = false;

    logger->debug("Broadcast register commands to probe HCC via Tx channel {}", tx);
    hwCtrl.disableCmd();
    hwCtrl.setCmdEnable(tx);

    // Toggle bit 2 in register Pulse to load serial number into register Addressing
    sendCommand(star.write_hcc_register(HCCStarRegister::Pulse, 0x4), hwCtrl);

    // Scan through the Rx channels and look for response
    for (auto rx : rxChannels) {
      // Read the register Addressing
      sendCommand(star.read_hcc_register(HCCStarRegister::Addressing), hwCtrl);

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
          uint32_t address = (nHCC << 28) | (fuseID & 0x00ffffff);
          sendCommand(star.write_hcc_register(HCCStarRegister::Addressing, address), hwCtrl);
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
    return false;
  }

  // Enable only the Tx and Rx channels that are connected to the HCCs
  enableConnectedChannels(hwCtrl, HCCs);

  return true;
}

bool probeABCs(HwController& hwCtrl, StarCfg& cfg, std::vector<Hybrid>& hccStars, bool reset) {
  bool hasABCStar = false;

  if (reset) {
    sendCommand(LCB::fast_command(LCB::ABC_REG_RESET, 0), hwCtrl);
  }

  for (auto& hcc : hccStars) {
    logger->info("Reading HPR packets from ABCStars on HCCStar {}", hcc.hcc_id);

    // Enable the tx and rx channel
    hwCtrl.setCmdEnable(hcc.tx);
    hwCtrl.setRxEnable(hcc.rx);

    // Toggle the TestHPR bit in case of the emulator or HPR was previously stopped
    auto [addr_sc, val_sc] = updateABCSubRegister("TESTHPR", 1, cfg);
    sendCommand(star.write_abc_register(addr_sc, val_sc), hwCtrl);

    unsigned activeInChannels = 0;

    std::function<bool(RawData&)> filter_abchpr = [&hcc](RawData& d) {
      return isPacketType(d, TYP_ABC_HPR) and isFromChannel(d, hcc.rx);
    };
    uint32_t timeout = 1000; // milliseconds

    auto rdc = readAllData(hwCtrl, filter_abchpr, timeout);

    for (unsigned c = 0; c < rdc.size(); c++) {
        RawDataPtr d = rdc.data[c];
      StarChipPacket packet;

      if ( packetFromRawData(packet, *d) ) {
        logger->error("Packet parse failed");
      } else {
        logger->trace(" Received an HPR packet from ABCStar");
        hasABCStar = true;

        // check the input channel
        uint32_t abc_chn = packet.channel_abc;
        // get the chipID from the top four bits of the 16-bit status word
        uint32_t abcid = (packet.abc_status >> 12) & 0xf;

        if ( hcc.abcs.find(abc_chn) == hcc.abcs.end() ) {
          // new channel
          hcc.abcs[abc_chn] = abcid;
          logger->info(" Received an HPR packet from the ABCStar on channel {} with chipID {}", abc_chn, abcid);
          activeInChannels |= (1 << abc_chn);

          // print the HPR packet
          std::stringstream os;
          packet.print_more(os);
          std::string str = os.str();
          str.erase(str.end()-1); // strip the extra \n
          logger->debug(" Received HPR packet: {}", str);
        }
      }

    } // end of data container loop

    if (not activeInChannels) {
      logger->warn("No ABCStar data from HCCStar {}", hcc.hcc_id);
    }

    // Update HCC register ICenable
    logger->debug("Set register ICenable on HCCStar {} to 0x{:08x}", hcc.hcc_id, activeInChannels);
    uint32_t addr_en = updateHCCRegister("ICenable", activeInChannels, cfg);
    sendCommand(star.write_hcc_register(addr_en, activeInChannels, hcc.hcc_id), hwCtrl);
  } // end of HCC loop

  if (not hasABCStar) {
    logger->error("No ABCStar from any HCCStar");
  }

  // Restore channel enable flags
  enableConnectedChannels(hwCtrl, hccStars);

  return hasABCStar;
}

bool testRegisterReadWrite(HwController& hwCtrl, uint32_t regAddr, uint32_t write_value, uint32_t rx, int hccId, int abcId=-1) {
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

bool testHCCRegisterAccess(HwController& hwCtrl, const std::vector<Hybrid>& hccStars) {
  logger->info("Test HCCStar register read & write");

  bool success = not hccStars.empty();

  for (const auto& hcc : hccStars) {
    // Register ErrCfg
    success &= testRegisterReadWrite(hwCtrl, HCCStarRegister::ErrCfg, 0xdeadbeef, hcc.rx, hcc.hcc_id);
  }

  return success;
}

bool testABCRegisterAccess(HwController& hwCtrl, StarCfg& cfg, const std::vector<Hybrid>& hccStars) {
  logger->info("Test ABCStar register read & write");

  // Set RR mode to 1
  auto [addr_rr, val_rr] = updateABCSubRegister("RRMODE", 1, cfg);
  sendCommand(star.write_abc_register(addr_rr, val_rr), hwCtrl);

  bool success = not hccStars.empty();

  for (const auto& hcc : hccStars) {
    if (hcc.abcs.empty())
      success = false;

    for (const auto& abc : hcc.abcs) {
      // Register MaskInput0
      success &= testRegisterReadWrite(hwCtrl, ABCStarRegister::MaskInput0, 0xabadcafe, hcc.rx, hcc.hcc_id, abc.second);
    }
  }

  return success;
}

bool testHitCounts(HwController& hwCtrl, StarCfg& cfg) {
  logger->info("Test ABCStar hit counters");

  // Enable hit counters
  logger->debug(" Enable hit counters and set TM to 1 (static test mode)");
  // TM: 1
  auto [addr_tm, val_tm] = updateABCSubRegister("TM", 1, cfg);
  sendCommand(star.write_abc_register(addr_tm, val_tm), hwCtrl);
  // RR mode: 1
  auto [addr_rr, val_rr] = updateABCSubRegister("RRMODE", 1, cfg);
  sendCommand(star.write_abc_register(addr_rr, val_rr), hwCtrl);
  // EnCount: 1
  auto [addr_cnt, val_cnt] = updateABCSubRegister("ENCOUNT", 1, cfg);
  sendCommand(star.write_abc_register(addr_cnt, val_cnt), hwCtrl);
  // LP_ENABLE: 0
  auto [addr_lp, val_lp] = updateABCSubRegister("LP_ENABLE", 0, cfg);
  sendCommand(star.write_abc_register(addr_lp, val_lp), hwCtrl);
  // PR_ENABLE: 0
  auto [addr_pr, val_pr] = updateABCSubRegister("PR_ENABLE", 0, cfg);
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
  sendCommand(star.read_abc_register(ABCStarRegister::HitCountREG63), hwCtrl);

  auto data = readData(
    hwCtrl,
    [](RawData& d) {return isPacketType(d, TYP_ABC_RR);}
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

bool testDataPacketsStatic(HwController& hwCtrl, StarCfg& cfg) {
  logger->info("Read ABCStar data packets in static mode");

  // Static test mode first
  logger->debug(" Set TM to 1 and enable LP and PR");
  // TM: 1
  auto [addr_tm, val_tm] = updateABCSubRegister("TM", 1, cfg);
  sendCommand(star.write_abc_register(addr_tm, val_tm), hwCtrl);
  // RR mode: 1
  auto [addr_rr, val_rr] = updateABCSubRegister("RRMODE", 1, cfg);
  sendCommand(star.write_abc_register(addr_rr, val_rr), hwCtrl);
  // EnCount: 0
  auto [addr_cnt, val_cnt] = updateABCSubRegister("ENCOUNT", 0, cfg);
  sendCommand(star.write_abc_register(addr_cnt, val_cnt), hwCtrl);
  // LP_ENABLE: 1
  auto [addr_lp, val_lp] = updateABCSubRegister("LP_ENABLE", 1, cfg);
  sendCommand(star.write_abc_register(addr_lp, val_lp), hwCtrl);
  // PR_ENABLE: 1
  auto [addr_pr, val_pr] = updateABCSubRegister("PR_ENABLE", 1, cfg);
  sendCommand(star.write_abc_register(addr_pr, val_pr), hwCtrl);

  // Enable PR & LP
  sendCommand(LCB::fast_command(LCB::HCC_START_PRLP, 0), hwCtrl);

  // BC Reset
  sendCommand(LCB::lonely_bcr(), hwCtrl);

  // Send a trigger
  sendCommand(LCB::l0a_mask(1, 233, false), hwCtrl);

  // Read the data packets
  auto rdc = readAllData(
    hwCtrl, [](RawData& d) {return isPacketType(d, TYP_LP);}
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

bool testDataPacketsPulse(HwController& hwCtrl, StarCfg& cfg) {
  logger->info("Read ABCStar data packets in test pulse mode");

  // Test pulse mode
  logger->debug(" Set TM to 2 and enable test pulse");
  // TM: 2
  auto [addr_tm, val_tm] = updateABCSubRegister("TM", 2, cfg);
  sendCommand(star.write_abc_register(addr_tm, val_tm), hwCtrl);
  // RR mode: 1
  auto [addr_rr, val_rr] = updateABCSubRegister("RRMODE", 1, cfg);
  sendCommand(star.write_abc_register(addr_rr, val_rr), hwCtrl);
  // EnCount: 0
  auto [addr_cnt, val_cnt] = updateABCSubRegister("ENCOUNT", 0, cfg);
  sendCommand(star.write_abc_register(addr_cnt, val_cnt), hwCtrl);
  // LP_ENABLE: 1
  auto [addr_lp, val_lp] = updateABCSubRegister("LP_ENABLE", 1, cfg);
  sendCommand(star.write_abc_register(addr_lp, val_lp), hwCtrl);
  // PR_ENABLE: 1
  auto [addr_pr, val_pr] = updateABCSubRegister("PR_ENABLE", 1, cfg);
  sendCommand(star.write_abc_register(addr_pr, val_pr), hwCtrl);
  // TEST_PULSE_ENABLE: 1
  auto [addr_tp, val_tp] = updateABCSubRegister("TEST_PULSE_ENABLE", 1, cfg);
  sendCommand(star.write_abc_register(addr_tp, val_tp), hwCtrl);

  // Set the L0 pipeline latency to a smaller value: 15
  uint32_t abc_latency = 15;
  logger->debug(" Set L0 latency to {}", abc_latency);
  auto [addr_lat, val_lat] = updateABCSubRegister("LATENCY", abc_latency, cfg);
  sendCommand(star.write_abc_register(addr_lat, val_lat), hwCtrl);

  // Set BCIDrstDelay of the HCC so we won't get BCID errors
  // L0 latency - 2 for ABCStar v0; L0 latency - 6 for ABCStar v1
  // Get ABC version from StarCfg?
  uint32_t bcdelay = abc_latency - 2;
  auto [addr_delay, val_delay] = updateHCCSubRegister("BCIDRSTDELAY", bcdelay, cfg);
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
    hwCtrl, [](RawData& d) {return isPacketType(d, TYP_LP);}
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

bool readABCRegisters(HwController& hwCtrl) {

  bool success = false;

  hwCtrl.flushBuffer();

  // Read an ABCStar HPR
  logger->info("Reading an HPR packet from ABCStar");
  auto data_abchpr = readData(
    hwCtrl,
    [](RawData& d) {return isPacketType(d, TYP_ABC_HPR);}
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
  sendCommand(star.read_abc_register(ABCStarRegister::MaskInput3), hwCtrl);
  auto data_abcrr = readData(
    hwCtrl,
    [](RawData& d) {return isPacketType(d, TYP_ABC_RR);}
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

} // end of unnamed namespace

//////////
int main(int argc, char *argv[]) {
    std::string controller;
    std::string controllerType;

    // Original Spec version
    std::vector<uint32_t> rxChannels = {6};
    std::vector<uint32_t> txChannels = {0xFFFF};
    bool setHccId = false;
    bool doResets = false;
    std::string testSequence("Full");
    unsigned inChannel = 0;
    std::string chipVersion("Star");

    // logger config path
    std::string logCfgPath = "";

    int c;
    while ((c = getopt(argc, argv, "hl:r:t:dRs:c:V:")) != -1) {
      switch(c) {
      case 'h':
        printHelp();
        return 0;
      case 'l':
        logCfgPath = std::string(optarg);
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
      case 'R':
        doResets = true;
        break;
      case 's':
        testSequence = std::string(optarg);
        break;
      case 'c':
        inChannel = atoi(optarg);
        if (inChannel > 11) {
          spdlog::error("Invalid HCC input channel: {}", inChannel);
          return 1;
        }
        break;
      case 'V':
        chipVersion = std::string(optarg);
        break;
      default:
        spdlog::critical("Error while parsing command line parameters!");
        return -1;
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

    // A global StarCfg with dummy chip configs
    StarCfg starCfg(abc_version, hcc_version);
    starCfg.setHCCChipId(0xf);
    starCfg.addABCchipID(0xf);

    // Controller
    if (optind != argc) {
      // First positional parameter (optind is first not parsed by getopt)
      controller = argv[optind];
    }

    std::unique_ptr<HwController> hwCtrl = nullptr;
    if(controller.empty()) {
	controllerType = "spec";
        hwCtrl = StdDict::getHwController(controllerType);
        // hwCtrl->init(0);
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

    hwCtrl->toggleTrigAbort();
    hwCtrl->setTrigEnable(0);

    // Enable Tx channels
    hwCtrl->setCmdEnable(txChannels);

    // Enable Rx channels
    hwCtrl->disableRx();
    hwCtrl->setRxEnable(rxChannels);

    // Tests
    bool success = false;

    std::vector<Hybrid> hccStars;
    /*
      Full test sequence
    */
    if (testSequence == "Full") {
      // Read HCCStar HPRs
      success = checkHPRs(*hwCtrl, rxChannels, doResets);

      // Probe HCCs
      success &= probeHCCs(*hwCtrl, hccStars, txChannels, rxChannels, setHccId);

      // Test HCCStar register read and write
      success &= testHCCRegisterAccess(*hwCtrl, hccStars);

      // Configure HCCs to enable communications with ABCs
      configureHCC(*hwCtrl, starCfg, doResets);

      // Probe ABCStars via reading ABCStar HPRs
      success &= probeABCs(*hwCtrl, starCfg, hccStars, doResets);

      // Test ABCStar register read and write
      success &= testABCRegisterAccess(*hwCtrl, starCfg, hccStars);

      // Configure ABCs
      configureABC(*hwCtrl, starCfg, doResets);

      // Read ABC hit counters
      success &= testHitCounts(*hwCtrl, starCfg);

      // Read ABC data packets
      success &= testDataPacketsStatic(*hwCtrl, starCfg);
      success &= testDataPacketsPulse(*hwCtrl, starCfg);
    }

    /*
      Test register read and write
    */
    else if (testSequence == "Register") {
      // Probe HCCs
      success = probeHCCs(*hwCtrl, hccStars, txChannels, rxChannels, setHccId);

      // Test HCCStar register read and write
      success &= testHCCRegisterAccess(*hwCtrl, hccStars);

      // Configure HCCs to enable communications with ABCs
      configureHCC(*hwCtrl, starCfg, doResets);

      // Check ABCStars
      success &= probeABCs(*hwCtrl, starCfg, hccStars, doResets);

      // Test ABCStar register read and write
      success &= testABCRegisterAccess(*hwCtrl, starCfg, hccStars);
    }

    /*
      Test reading data packets
    */
    else if (testSequence == "DataPacket") {
      // Probe HCCs
      success = probeHCCs(*hwCtrl, hccStars, txChannels, rxChannels, setHccId);

      // Configure HCCs to enable communications with ABCs
      configureHCC(*hwCtrl, starCfg, doResets);

      // Check ABCStars
      success &= probeABCs(*hwCtrl, starCfg, hccStars, doResets);

      // Configure ABCs
      configureABC(*hwCtrl, starCfg, doResets);

      // Read ABC data packets
      success &= testDataPacketsStatic(*hwCtrl, starCfg);
      success &= testDataPacketsPulse(*hwCtrl, starCfg);
    }

    /*
      Probe the front end ASICs
    */
    else if (testSequence == "Probe") {
      // Read HCCStar HPRs
      success = checkHPRs(*hwCtrl, rxChannels, doResets);

      // Probe HCCs
      success &= probeHCCs(*hwCtrl, hccStars, txChannels, rxChannels, setHccId);

      if (doResets) {
        // In case resets were sent, HCCs need to be reconfigured to talk to ABCs
        configureHCC(*hwCtrl, starCfg, doResets);
      }

      // Probe ABCStars via reading ABCStar HPRs
      success &= probeABCs(*hwCtrl, starCfg, hccStars, doResets);
    }

    /*
      Run some tests in packet transparent mode
    */
    else if (testSequence == "PacketTransp") {
      // configure HCC into the Packet Transparent mode
      configureHCC_PacketTransp(*hwCtrl, starCfg, doResets);

      // configure ABCs
      configureABC(*hwCtrl, starCfg, doResets);

      // read and print some ABC registers
      success = readABCRegisters(*hwCtrl);

      // read and print some data packets
      success &= testDataPacketsStatic(*hwCtrl, starCfg
);
    }

    /*
      Run some tests in full transparent mode
    */
    else if (testSequence == "FullTransp") {
      configureHCC_FullTransp(*hwCtrl, starCfg, doResets, inChannel);
      configureABC(*hwCtrl, starCfg, doResets);

      // read and print some ABC registers
      success = readABCRegisters(*hwCtrl);

      // read and print some data packets
      success &= testDataPacketsStatic(*hwCtrl, starCfg);

      /*
      // try reading everything for 1 seconds
      auto rdc = readAllData(
        *hwCtrl,
        [](RawData& d) {return true;}, // no filter on data packet type
        1000 // ms
        );

      for (unsigned c = 0; c < rdc.size(); c++) {
        RawData d(rdc.adr[c], rdc.buf[c], rdc.words[c]);
        reportData(d);
      }
      */
    }

    else {
      logger->error("Unknown test sequence: {}", testSequence);
      logger->info("Available test presets are: Full, Register, DataPacket, Probe, PacketTransp, FullTransp");
      success = false;
    }

    hwCtrl->disableRx();

    if (not success) {
      logger->error("Tests failed");
      return 1;
    }

    logger->info("Success!");
    return 0;
}
