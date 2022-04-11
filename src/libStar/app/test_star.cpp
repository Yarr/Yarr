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
    uint32_t tx;
    uint32_t rx;
    uint32_t hcc_id;
    std::map<uint32_t,uint32_t> abcs; // key: channel; value: chipID
  };
}

static void printHelp() {
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

std::shared_ptr<RawData> readData(
  HwController& hwCtrl,
  std::function<bool(RawData&)> filter_cb,
  uint32_t timeout=1000)
{
  bool nodata = true;

  std::vector<std::pair<uint32_t, std::shared_ptr<RawData>>> dataVec = hwCtrl.readData();
  std::shared_ptr<RawData> data;
  if (dataVec.size() > 0)
      data = dataVec[0].second;

  auto start_reading = std::chrono::steady_clock::now();

  while (true) {
    if (data) {
      nodata = false;
      logger->trace("Use data: {}", (void*)data->getBuf());

      // check if it is the type of data we want
      bool good = filter_cb(*data);
      if (good) {
        break;
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

    dataVec = hwCtrl.readData();
    if (dataVec.size() > 0) {
      data = dataVec[0].second;
    } else {
      data = nullptr;
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

  std::vector<std::pair<uint32_t, std::shared_ptr<RawData>>> dataVec = hwCtrl.readData();
  std::shared_ptr<RawData> data;
  if (dataVec.size() > 0)
      data = dataVec[0].second;

  auto start_reading = std::chrono::steady_clock::now();

  while (true) {
    if (data) {
      nodata = false;
      logger->trace("Use data: {}", (void*)data->getBuf());

      bool good = filter_cb(*data);
      if (good) {
        rdc.add(std::move(data));
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

      if (nodata) {
        logger->critical("No data");
      } else if (not data) {
        logger->debug("No data met the requirement");
      }
  }

  if (nodata) {
    logger->critical("No data");
  } else if (rdc.size() == 0) {
    logger->debug("Data container is empty");
  }

  return rdc;
}

void reportData(RawData &data, bool do_spec_specific=false) {
  logger->info(" Raw data from RxCore:");
  logger->info(" {} {:p} {}", data.getAdr(), (void*)data.getBuf(), data.getSize());

  for (unsigned j=0; j<data.getSize();j++) {
    auto word = data[j];

    if(do_spec_specific) {
      if((j%2) && (word == 0xd3400000)) continue;
      if(!(j%2) && ((word&0xff) == 0xff)) continue;

      if((word&0xff) == 0x5f) continue;

      if(word == 0x1a0d) continue; // Idle on chan 6
      if(word == 0x19f2) continue; // Idle on chan 6

      word &= 0xffffc3ff; // Strip of channel number
    }

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
void configureHCC(HwController& hwCtrl, bool reset) {
  // Configure HCCStars to enable communications with ABCStars
  if (reset) {
    logger->info("Sending HCCStar register reset command");
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

  if (reset) {
    // Register 45/46: external reset for ABCStars
    sendCommand(star.write_hcc_register(45, 0x00000001), hwCtrl);
    sendCommand(star.write_hcc_register(46, 0x00000001), hwCtrl);
  }
}

void configureHCC_PacketTransp(HwController& hwCtrl, bool reset) {
  configureHCC(hwCtrl, reset);

  // Set to packet transparent mode
  logger->info("Set HCCs to Packet Transparent mode");
  sendCommand(star.write_hcc_register(41, 0x00020201), hwCtrl);
  sendCommand(star.write_hcc_register(42, 0x00020201), hwCtrl);
}

void configureHCC_FullTransp(HwController& hwCtrl, bool reset, unsigned inChn) {
  configureHCC(hwCtrl, reset);

  // Select the input channel: IC_transSelect
  // Register 40
  inChn = inChn & 0xf;
  unsigned value_reg40 = (inChn << 16) + (1 << inChn);
  sendCommand(star.write_hcc_register(40, value_reg40), hwCtrl);

  // Set to full transparent mode
  logger->info("Set HCCs to Full Transparent mode");
  sendCommand(star.write_hcc_register(41, 0x00020301), hwCtrl);
  sendCommand(star.write_hcc_register(42, 0x00020301), hwCtrl);
}

void configureABC(HwController& hwCtrl, bool reset) {

  if (reset) {
    logger->info("Sending ABCStar register reset commands");
    sendCommand(LCB::fast_command(LCB::ABC_REG_RESET, 0), hwCtrl);
    sendCommand(LCB::fast_command(LCB::ABC_SLOW_COMMAND_RESET, 0), hwCtrl);
  }

  logger->info("Broadcast ABCStar configurations");

  // Register 32 (CREG0): set RR mode to 1, enable LP and PR
  sendCommand(star.write_abc_register(32, 0x00000700), hwCtrl);

  // Set some mask registers to some nonzero value
  logger->debug(" Set MaskInput3 to 0xfffe0000");
  sendCommand(star.write_abc_register(19, 0xfffe0000), hwCtrl); // MaskInput3
  logger->debug(" Set MaskInput7 to 0xff000000");
  sendCommand(star.write_abc_register(23, 0xff000000), hwCtrl); // MaskInput7
}

// Test steps
bool checkHPRs(HwController& hwCtrl, const std::vector<uint32_t>& rxChannels,
               bool reset)
{
  if (reset) {
    sendCommand( LCB::fast_command(LCB::HCC_REG_RESET, 0), hwCtrl );
  }

  uint32_t timeout = 1000; // milliseconds

  bool hprOK = false;

  // Should not be necessary except for the emulator, but toggle the TestHPR
  // bit anyway in case HPR was stopped previously
  sendCommand(star.write_hcc_register(16, 0x2), hwCtrl);

  for (auto rx : rxChannels) {
    logger->info("Reading HPR packets from Rx channel {}", rx);

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
    return false;
  }

  return true;
}

bool probeABCs(HwController& hwCtrl, std::vector<Hybrid>& hccStars, bool reset) {
  bool hasABCStar = false;

  if (reset) {
    sendCommand(LCB::fast_command(LCB::ABC_REG_RESET, 0), hwCtrl);
  }

  // Toggle the TestHPR bit in case of the emulator or HPR was previously stopped
  sendCommand(star.write_abc_register(0, 0x8), hwCtrl);

  for (auto& hcc : hccStars) {
    logger->info("Reading HPR packets from ABCStars on HCCStar {}", hcc.hcc_id);

    unsigned activeInChannels = 0;

    std::function<bool(RawData&)> filter_abchpr = [&hcc](RawData& d) {
      return isPacketType(d, TYP_ABC_HPR) and isFromChannel(d, hcc.rx);
    };
    uint32_t timeout = 1000; // milliseconds

    auto rdc = readAllData(hwCtrl, filter_abchpr, timeout);

    for (unsigned c = 0; c < rdc.size(); c++) {
        std::shared_ptr<RawData> d = rdc.data[c];
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

    // Update HCC register 40 ICenable
    logger->debug("Set register 40 (ICenable) on HCCStar {} to 0x{:08x}", hcc.hcc_id, activeInChannels);
    sendCommand(star.write_hcc_register(40, activeInChannels, hcc.hcc_id), hwCtrl);
  } // end of HCC loop

  if (not hasABCStar) {
    logger->error("No ABCStar from any HCCStar");
  }

  return hasABCStar;
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

bool testHCCRegisterAccess(HwController& hwCtrl, const std::vector<Hybrid>& hccStars) {
  logger->info("Test HCCStar register read & write");

  bool success = not hccStars.empty();

  for (const auto& hcc : hccStars) {
    // Register 47: ErrCfg
    success &= testRegisterReadWrite(hwCtrl, 47, 0xdeadbeef, hcc.rx, hcc.hcc_id);
  }

  return success;
}

bool testABCRegisterAccess(HwController& hwCtrl, const std::vector<Hybrid>& hccStars) {
  logger->info("Test ABCStar register read & write");

  // Set RR mode to 1
  sendCommand(star.write_abc_register(32, 0x00000400), hwCtrl);

  bool success = not hccStars.empty();

  for (const auto& hcc : hccStars) {
    if (hcc.abcs.empty())
      success = false;

    for (const auto& abc : hcc.abcs) {
      // Register 16: MaskInput0
      success &= testRegisterReadWrite(hwCtrl, 16, 0xabadcafe, hcc.rx, hcc.hcc_id, abc.second);
    }
  }

  return success;
}

bool testHitCounts(HwController& hwCtrl) {
  logger->info("Test ABCStar hit counters");

  // Enable hit counters
  logger->debug(" Enable hit counters and set TM to 1 (static test mode)");
  // TM: 1; RR mode: 1; LP and PR enabled; EnCount: 1;
  sendCommand(star.write_abc_register(32, 0x00010720), hwCtrl);

  // Reset and start ABCStar hit counters
  logger->debug(" Reset hit counters");
  sendCommand(LCB::fast_command(LCB::ABC_HIT_COUNT_RESET, 0), hwCtrl);
  logger->debug(" Start hit counters");
  sendCommand(LCB::fast_command(LCB::ABC_HIT_COUNT_START, 0), hwCtrl);

  // Enable PR & LP
  logger->debug(" Start PRLP");
  sendCommand(LCB::fast_command(LCB::HCC_START_PRLP, 0), hwCtrl);

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
  logger->debug(" Read register 191: HitCountREG63");
  sendCommand(star.read_abc_register(191), hwCtrl);

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

bool testDataPacketsStatic(HwController& hwCtrl, bool do_spec_specific) {
  logger->info("Read ABCStar data packets in static mode");

  // Static test mode first
  logger->debug(" Set TM to 1");
  // TM: 1; RR mode: 1; LP and PR enabled
  sendCommand(star.write_abc_register(32, 0x00010700), hwCtrl);

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
    std::shared_ptr<RawData> d = rdc.data[c];
    reportData(*d, do_spec_specific);
  }

  if (rdc.size() > 0) {
    logger->info("Read data packets in static test mode: Ok");
  } else {
    logger->error("Fail to read data packets in static test mode");
    return false;
  }

  return true;
}

bool testDataPacketsPulse(HwController& hwCtrl, bool do_spec_specific) {
  logger->info("Read ABCStar data packets in test pulse mode");

  // Test pulse mode
  logger->debug(" Set TM to 2 and enable test pulse");
  // TM: 2; RR mode: 1; LP and PR enabled; TestPulseEnable: 1
  sendCommand(star.write_abc_register(32, 0x00020710), hwCtrl);

  // Set the L0 pipeline latency to a smaller value: 15
  logger->debug(" Set L0 latency to 15");
  sendCommand(star.write_abc_register(34, 0x0000000f), hwCtrl);

  // Set BCIDrstDelay of the HCC to L0 latency - 2 so we won't get BCID errors
  sendCommand(star.write_hcc_register(44, 0x0000000d), hwCtrl);

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
    std::shared_ptr<RawData> d = rdc.data[c];
    reportData(*d, do_spec_specific);
  }

  if (rdc.size() > 0) {
    logger->info("Read data packets in test pulse mode: Ok");
  } else {
    logger->error("Fail to read data packets in test pulse mode");
    return false;
  }

  return true;
}

bool readABCRegisters(HwController& hwCtrl, bool do_spec_specific=false) {

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
    reportData(*data_abchpr, do_spec_specific);
    success = true;
  } else {
    logger->error("No ABCStar HPR packet received.");
    success = false;
  }

  // Read ABCStar register 19: MaskInput3
  logger->info("Reading ABCStar register 19 (MaskInput3)");
  sendCommand(star.read_abc_register(19), hwCtrl);
  auto data_abcrr = readData(
    hwCtrl,
    [](RawData& d) {return isPacketType(d, TYP_ABC_RR);}
    );
  if (data_abcrr) {
    logger->info("Received an ABCStar RR packet.");
    reportData(*data_abcrr, do_spec_specific);
    success &= true;
  } else {
    logger->error("No ABCStar RR packet received.");
    success = false;
  }

  return success;
}

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

    // logger config path
    std::string logCfgPath = "";

    int c;
    while ((c = getopt(argc, argv, "hl:r:t:dRs:c:")) != -1) {
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
      configureHCC(*hwCtrl, doResets);

      // Probe ABCStars via reading ABCStar HPRs
      success &= probeABCs(*hwCtrl, hccStars, doResets);

      // Test ABCStar register read and write
      success &= testABCRegisterAccess(*hwCtrl, hccStars);

      // Configure ABCs
      configureABC(*hwCtrl, doResets);

      // Read ABC hit counters
      success &= testHitCounts(*hwCtrl);

      // Read ABC data packets
      success &= testDataPacketsStatic(*hwCtrl, controllerType=="spec");
      success &= testDataPacketsPulse(*hwCtrl, controllerType=="spec");
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
      configureHCC(*hwCtrl, doResets);

      // Check ABCStars
      success &= probeABCs(*hwCtrl, hccStars, doResets);

      // Test ABCStar register read and write
      success &= testABCRegisterAccess(*hwCtrl, hccStars);
    }

    /*
      Test reading data packets
    */
    else if (testSequence == "DataPacket") {
      // Probe HCCs
      success = probeHCCs(*hwCtrl, hccStars, txChannels, rxChannels, setHccId);

      // Configure HCCs to enable communications with ABCs
      configureHCC(*hwCtrl, doResets);

      // Check ABCStars
      success &= probeABCs(*hwCtrl, hccStars, doResets);

      // Configure ABCs
      configureABC(*hwCtrl, doResets);

      // Read ABC data packets
      success &= testDataPacketsStatic(*hwCtrl, controllerType=="spec");
      success &= testDataPacketsPulse(*hwCtrl, controllerType=="spec");
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
        configureHCC(*hwCtrl, doResets);
      }

      // Probe ABCStars via reading ABCStar HPRs
      success &= probeABCs(*hwCtrl, hccStars, doResets);
    }

    /*
      Run some tests in packet transparent mode
    */
    else if (testSequence == "PacketTransp") {
      // configure HCC into the Packet Transparent mode
      configureHCC_PacketTransp(*hwCtrl, doResets);

      // configure ABCs
      configureABC(*hwCtrl, doResets);

      // read and print some ABC registers
      success = readABCRegisters(*hwCtrl, controllerType=="spec");

      // read and print some data packets
      success &= testDataPacketsStatic(*hwCtrl, controllerType=="spec");
    }

    /*
      Run some tests in full transparent mode
    */
    else if (testSequence == "FullTransp") {
      configureHCC_FullTransp(*hwCtrl, doResets, inChannel);
      configureABC(*hwCtrl, doResets);

      // read and print some ABC registers
      success = readABCRegisters(*hwCtrl, controllerType=="spec");

      // read and print some data packets
      success &= testDataPacketsStatic(*hwCtrl, controllerType=="spec");

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
