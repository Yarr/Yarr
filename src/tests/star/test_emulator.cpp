#include "catch.hpp"

#include "LCBUtils.h"
#include "StarCmd.h"
#include "StarChipPacket.h"
#include "AllHwControllers.h"

void sendCommand(TxCore &hw, std::array<uint16_t, 9> &cmd) {
  hw.writeFifo((LCB::IDLE << 16) + LCB::IDLE);
  hw.writeFifo((cmd[0] << 16) + cmd[1]);
  hw.writeFifo((cmd[2] << 16) + cmd[3]);
  hw.writeFifo((cmd[4] << 16) + cmd[5]);
  hw.writeFifo((cmd[6] << 16) + cmd[7]);
  hw.writeFifo((cmd[8] << 16) + LCB::IDLE);
}

template<typename PacketT>
void compareOutputs(RawData* data, const PacketT& expected_packet);

template<typename PacketT>
void checkData(HwController*, std::deque<PacketT>&, const PacketT&);

// Test by parsing bytes and comparing string
TEST_CASE("StarEmulatorParsing", "[star][emulator]") {
  std::shared_ptr<HwController> emu = StdDict::getHwController("emu_Star");

  REQUIRE (emu);

  json cfg;
  emu->loadConfig(cfg);

  emu->setCmdEnable(0xFFFF);
  emu->setRxEnable(0x0);

  StarCmd star;

  typedef std::string PacketCompare;

  // What data to expect, and how to mask the comparison
  std::deque<PacketCompare> expected;

  SECTION("Read HCCStar interposed") {
    // read another HCCStar register
    std::array<LCB::Frame, 9> readHCCCmd2 = star.read_hcc_register(17);
    emu->writeFifo((readHCCCmd2[0] << 16) + readHCCCmd2[1]);
    // the read command is interupted by an L0A
    emu->writeFifo((LCB::l0a_mask(1, 0, false) << 16) + readHCCCmd2[2]);
    emu->writeFifo((readHCCCmd2[8] << 16) + LCB::IDLE);

    // Response from L0?
    expected.push_back("Packet type TYP_LP, BCID 0 (0), L0ID 3, nClusters 0\n");
    // NB this is incorrect?
    expected.push_back("Packet type TYP_HCC_RR, ABC 0, Address 11, Value 00000000\n");
  }

  SECTION("Read counter register") {
    // read an ABCStar register with broadcast addresses
    // Reading hit counter register
    std::array<LCB::Frame, 9> readABCCmd = star.read_abc_register(172);
    sendCommand(*emu, readABCCmd);

    expected.push_back("Packet type TYP_ABC_RR, ABC 0, Address ac, Value 00000000\n");
  }

  emu->releaseFifo();

  while(!emu->isCmdEmpty())
    ;

  checkData(emu.get(), expected, std::string(""));

  emu->setRxEnable(0x0);
}

TEST_CASE("StarEmulatorBytes", "[star][emulator]") {
  std::shared_ptr<HwController> emu = StdDict::getHwController("emu_Star");

  REQUIRE (emu);

  json cfg;
  emu->loadConfig(cfg);

  emu->setCmdEnable(0xFFFF);
  emu->setRxEnable(0x0);

  StarCmd star;

  typedef std::vector<uint8_t> PacketCompare;

  // What data to expect, and how to mask the comparison
  std::deque<PacketCompare> expected;

  // Use the pattern below to skip a comparison
  // 0xf is not a valid packet type
  const PacketCompare mask_pattern = {0xff, 0xde, 0xad, 0xbe, 0xef, 0x00};

  //////////////////////////
  // Initialize the emulator
  //////////////////////////

  // Send reset fast commands
  emu->writeFifo((LCB::IDLE << 16) + LCB::fast_command(LCB::LOGIC_RESET, 0));
  emu->writeFifo((LCB::IDLE << 16) + LCB::fast_command(LCB::ABC_REG_RESET, 0));
  emu->writeFifo((LCB::IDLE << 16) + LCB::fast_command(LCB::HCC_REG_RESET, 0));

  // Turn off both HCC and ABC HPRs so HPRs will not interfere with other tests
  // HCC MaskHPR on
  std::array<LCB::Frame, 9> writeHCCCmd_MaskHPROn = star.write_hcc_register(43, 0x00000100);
  sendCommand(*emu, writeHCCCmd_MaskHPROn);
  // HCC StopHPR on
  std::array<LCB::Frame, 9> writeHCCCmd_StopHPROn = star.write_hcc_register(16, 0x00000001);
  sendCommand(*emu, writeHCCCmd_StopHPROn);
  // ABC MaskHPR on
  std::array<LCB::Frame, 9> writeABCCmd_MaskHPROn = star.write_abc_register(32, 0x00000040);
  sendCommand(*emu, writeABCCmd_MaskHPROn);
  // ABC StopHPR on
  std::array<LCB::Frame, 9> writeABCCmd_StopHPROn = star.write_abc_register(0, 0x00000004);
  sendCommand(*emu, writeABCCmd_StopHPROn);

  // Will still receive one initial HPR packet from HCC and one from each ABC
  // HCC HPR with Idle frame
  expected.push_back({0xe0, 0xf7, 0x85, 0x50, 0x02, 0xb0});
  // ABC HPR with Idle frame
  // (By default the emulator has only one hard-coded ABC with ID = 15 for now)
  expected.push_back({0xd0, 0x3f, 0x07, 0x85, 0x55, 0xff, 0xff, 0x00, 0x00});

  //////////////////////////
  // Start tests
  //////////////////////////

  SECTION("Read HCCStar") {
    // Read an HCCStar register
    std::array<LCB::Frame, 9> readHCCCmd = star.read_hcc_register(48); // 0x30
    sendCommand(*emu, readHCCCmd);

    // HCCStar register 48 (ADCcfg) is initialized to 0x00406600 
    expected.push_back({0x83, 0x00, 0x04, 0x06, 0x60, 0x00});
  }

  SECTION("Read HCCStar short") {
    // Read an HCCStar register using only 4 words
    std::array<LCB::Frame, 9> readHCCCmd = star.read_hcc_register(44); // 0x2c
    emu->writeFifo((readHCCCmd[0] << 16) + readHCCCmd[1]);
    emu->writeFifo((readHCCCmd[2] << 16) + readHCCCmd[8]);

    // HCCStar register 44 (Cfg2) is initialized to 0x0000018e
    expected.push_back({0x82, 0xc0, 0x00, 0x00, 0x18, 0xe0});
  }

  SECTION("Read ABCStar interposed") {
    // Read an ABCStar register
    std::array<LCB::Frame, 9> readABCCmd =  star.read_abc_register(34); // 0x22
    emu->writeFifo((readABCCmd[0] << 16) + readABCCmd[1]);
    // The read command is interupted by an L0A
    emu->writeFifo((LCB::l0a_mask(1, 0, false) << 16) + readABCCmd[2]);
    emu->writeFifo((readABCCmd[8] << 16) + LCB::IDLE);

    // Response from L0A: empty cluster; l0tag = 0 + 3; bcid = 0b0000
    expected.push_back({0x20, 0x30, 0x03, 0xfe, 0x6f, 0xed});
    // ABCStar register 34 (CREG2): 0x00000190
    expected.push_back({0x40, 0x22, 0x00, 0x00, 0x00, 0x19, 0x0f, 0x00, 0x00});
  }

  SECTION("Mask Registers") {
    // Switch to static test mode: TM = 1
    std::array<LCB::Frame, 9> writeABCCmd_TM = star.write_abc_register(32, 0x00010040);
    sendCommand(*emu, writeABCCmd_TM);

    // Set mask registers
    std::array<LCB::Frame, 9> writeABCCmd_MaskInput3 = star.write_abc_register(19, 0xfffe0000);
    sendCommand(*emu, writeABCCmd_MaskInput3);
    std::array<LCB::Frame, 9> writeABCCmd_MaskInput7 = star.write_abc_register(23, 0xfffe0000);
    sendCommand(*emu, writeABCCmd_MaskInput7);

    // Send an L0A
    emu->writeFifo((LCB::IDLE << 16) + LCB::l0a_mask(1, 4, false));

    // l0tag = 4 + 3; bcid = 0b0111;
    expected.push_back({0x20, 0x77, 0x05, 0xc7, 0x01, 0xcf, 0x05, 0xe7, 0x01, 0xee, 0x07, 0xc7, 0x03, 0xcf, 0x07, 0xe7, 0x03, 0xee, 0x6f, 0xed});
  }

  SECTION("Hit Counters") {
    // Switch to static test mode (TM = 1) and enable hit counters
    std::array<LCB::Frame, 9> writeABCCmd_TM = star.write_abc_register(32, 0x00010060);
    sendCommand(*emu, writeABCCmd_TM);

    // Set a mask register
    std::array<LCB::Frame, 9> writeABCCmd_MaskInput0 = star.write_abc_register(16, 0xffffffff);
    sendCommand(*emu, writeABCCmd_MaskInput0);

    // Reset and start hit counters:
    emu->writeFifo((LCB::fast_command(LCB::ABC_HIT_COUNT_RESET, 0) << 16) + LCB::fast_command(LCB::ABC_HIT_COUNT_START, 0));

    // Send four triggers
    emu->writeFifo((LCB::l0a_mask(10, 8, false) << 16) + LCB::l0a_mask(10, 12, false));

    // Stop hit counters
    emu->writeFifo((LCB::IDLE << 16) + LCB::fast_command(LCB::ABC_HIT_COUNT_STOP, 0));

    // Send another trigger: it should not increase any hit counters
    emu->writeFifo((LCB::l0a_mask(1, 16, false) << 16) + LCB::IDLE);

    for (int i = 0; i < 5; i++) {
      // Skip the comparison of these cluster packets
      // Test of physics packets is done elsewhere
      expected.push_back(mask_pattern);
    }

    // Check the hit counts
    // HitCountREG0
    std::array<LCB::Frame, 9> readABCCmd_hitcnt0 = star.read_abc_register(128);
    emu->writeFifo((readABCCmd_hitcnt0[0] << 16) + readABCCmd_hitcnt0[1]);
    emu->writeFifo((readABCCmd_hitcnt0[2] << 16) + readABCCmd_hitcnt0[8]);
    // HitCountREG0 for channel 0 to 3 is expected to be 0x04040404
    expected.push_back({0x40, 0x80, 0x00, 0x40, 0x40, 0x40, 0x4f, 0x00, 0x00});

    // HitCountREG63
    std::array<LCB::Frame, 9> readABCCmd_hitcnt63 = star.read_abc_register(191);
    emu->writeFifo((readABCCmd_hitcnt63[0] << 16) + readABCCmd_hitcnt63[1]);
    emu->writeFifo((readABCCmd_hitcnt63[2] << 16) + readABCCmd_hitcnt63[8]);
    // HitCountREG63 for channel 252 - 255 is expected be 0x00000000
    expected.push_back({0x40, 0xbf, 0x00, 0x00, 0x00, 0x00, 0x0f, 0x00, 0x00});
  }

  SECTION("L0 Latency") {
    // Switch to test pulse mode: TM = 2, TestPulseEnable = 1
    std::array<LCB::Frame, 9> writeABCCmd_cfg = star.write_abc_register(32, 0x00020050);
    sendCommand(*emu, writeABCCmd_cfg);

    // Set a mask register so we are expecting a non-empty cluster packet
    std::array<LCB::Frame, 9> writeABCCmd_mask = star.write_abc_register(16, 0x00000001);
    sendCommand(*emu, writeABCCmd_mask);

    // Configure trigger latency to be 3 BC
    std::array<LCB::Frame, 9> writeABCCmd_lat = star.write_abc_register(34, 0x00000003);
    sendCommand(*emu, writeABCCmd_lat);

    // Send a digital pulse command, followed by an L0A three BC later
    emu->writeFifo((LCB::IDLE << 16) + LCB::IDLE);
    emu->writeFifo((LCB::fast_command(LCB::ABC_DIGITAL_PULSE, 0) << 16) + LCB::l0a_mask(1, 20, false));

    // Physics packet: l0tag = 20 + 3; bcid = 0b1000
    expected.push_back({0x21, 0x78, 0x00, 0x00, 0x6f, 0xed});
  }

  emu->releaseFifo();

  while(!emu->isCmdEmpty())
    ;

  checkData(emu.get(), expected, mask_pattern);

  emu->setRxEnable(0x0);
}

TEST_CASE("StarEmulatorHPR", "[star][emulator]") {
  std::shared_ptr<HwController> emu =  StdDict::getHwController("emu_Star");

  REQUIRE (emu);

  json cfg;
  cfg["hprPeriod"] = 80; // Set HPR period to 80
  emu->loadConfig(cfg);

  StarCmd star;

  typedef std::vector<uint8_t> PacketCompare;
  const PacketCompare mask_pattern = {0xff, 0xde, 0xad, 0xbe, 0xef, 0x00};

  std::deque<PacketCompare> expected;

  // Send a logic reset first
  emu->writeFifo((LCB::IDLE << 16) + LCB::fast_command(LCB::LOGIC_RESET, 0));

  // Wait for the initial HPRs, which should arrive 80/2 BC after reset
  // Send Idle frames to keep the "clock" in the emulator running
  for (int j = 0; j < 6; j++) {
    emu->writeFifo((LCB::IDLE << 16) + LCB::IDLE);
  }

  // HCC HPR with Idle frame
  expected.push_back({0xe0, 0xf7, 0x85, 0x50, 0x02, 0xb0});
  // ABC HPR with Idle frame
  expected.push_back({0xd0, 0x3f, 0x07, 0x85, 0x55, 0xff, 0xff, 0x00, 0x00});

  SECTION("Periodic") {
    // Wait another 80 BCs for a second set of HPRs
    for (int j = 0; j < 10; j++) {
      emu->writeFifo((LCB::IDLE << 16) + LCB::IDLE);
    }

    expected.push_back({0xe0, 0xf7, 0x85, 0x50, 0x02, 0xb0});
    expected.push_back({0xd0, 0x3f, 0x07, 0x85, 0x55, 0xff, 0xff, 0x00, 0x00});
  }

  SECTION("StopHPR") {
    // Set HCC StopHPR bit to 1 to stop periodic HPR packets from HCCStar
    std::array<LCB::Frame, 9> writeHCCCmd_StopHPR = star.write_hcc_register(16, 0x00000001);
    sendCommand(*emu, writeHCCCmd_StopHPR);

    // Wait a bit, and we should only see a periodic HPR packet from ABCStar
    for (int j = 0; j < 4; j++) {
      emu->writeFifo((LCB::IDLE << 16) + LCB::IDLE);
    }

    expected.push_back({0xd0, 0x3f, 0x07, 0x85, 0x55, 0xff, 0xff, 0x00, 0x00});
  }

  SECTION("TestHPR") {
    // Set ABC TestHPR bit to 1 to receive an ABC HPR packet immediately
    std::array<LCB::Frame, 9> writeABCCmd_TestHPR = star.write_abc_register(0, 0x00000008);
    sendCommand(*emu, writeABCCmd_TestHPR);

    expected.push_back({0xd0, 0x3f, 0x07, 0x85, 0x55, 0xff, 0xff, 0x00, 0x00});

    // Wait a bit, and there should be the regular periodic HPR packets
    for (int j = 0; j < 4; j++) {
      emu->writeFifo((LCB::IDLE << 16) + LCB::IDLE);
    }

    expected.push_back({0xe0, 0xf7, 0x85, 0x50, 0x02, 0xb0});
    expected.push_back({0xd0, 0x3f, 0x07, 0x85, 0x55, 0xff, 0xff, 0x00, 0x00});
  }

  SECTION("MaskHPR") {
    // Set HCC MaskHPR bit to 1
    std::array<LCB::Frame, 9> writeHCCCmd_MaskHPR = star.write_hcc_register(43, 0x00000100);
    sendCommand(*emu, writeHCCCmd_MaskHPR);

    // Wait a bit for the periodic HPR packets
    for (int j = 0; j < 4; j++) {
      emu->writeFifo((LCB::IDLE << 16) + LCB::IDLE);
    }

    expected.push_back({0xe0, 0xf7, 0x85, 0x50, 0x02, 0xb0});
    expected.push_back({0xd0, 0x3f, 0x07, 0x85, 0x55, 0xff, 0xff, 0x00, 0x00});

    // Now set HCC TestHPR bit to 1.
    // No extra HCC HPR is expected since the HCC MaskHPR is on
    std::array<LCB::Frame, 9> writeHCCCmd_TestHPR = star.write_hcc_register(16, 0x00000002);
    sendCommand(*emu, writeHCCCmd_TestHPR);

    // Wait a bit, and there should still be periodic HPRs from both HCC and ABC
    for (int j = 0; j < 4; j++) {
      emu->writeFifo((LCB::IDLE << 16) + LCB::IDLE);
    }

    expected.push_back({0xe0, 0xf7, 0x85, 0x50, 0x02, 0xb0});
    expected.push_back({0xd0, 0x3f, 0x07, 0x85, 0x55, 0xff, 0xff, 0x00, 0x00});
  }

  emu->releaseFifo();

  while(!emu->isCmdEmpty());

  checkData(emu.get(), expected, mask_pattern);
}

template<typename PacketT>
void checkData(HwController* emu, std::deque<PacketT>& expected, const PacketT& mask_pattern)
{
  std::unique_ptr<RawData> data(emu->readData());

  for(int reads=0; reads<10; reads++) {
    CAPTURE (reads);
    if(data) {
      CHECK (!expected.empty());

      PacketT expected_packet;
      if (!expected.empty()) {
        expected_packet = expected.front();
        expected.pop_front();
      }

      CAPTURE (data->words);

      // Do comparison
      if (expected_packet != mask_pattern)
        compareOutputs(data.get(), expected_packet);

      delete [] data->buf;
    }

    data.reset(emu->readData());
  }
}

template<>
void compareOutputs<std::string>(RawData* data, const std::string& expected_packet)
{
  StarChipPacket packet;
  packet.add_word(0x13c); //add SOP
  for(unsigned iw=0; iw<data->words; iw++) {
    for (int i=0; i<4;i++){
      packet.add_word((data->buf[iw]>>i*8)&0xff);
    }
  }
  packet.add_word(0x1dc); //add EOP

  bool parse_failed = packet.parse();
  std::stringstream ss;
  packet.print_words(ss);
  CAPTURE (ss.str());
  CHECK (!parse_failed);

  std::stringstream parsed;
  packet.print_more(parsed);

  CHECK (parsed.str() == expected_packet);
}

template<>
void compareOutputs<std::vector<uint8_t>>(RawData* data, const std::vector<uint8_t>& expected_packet)
{
  CAPTURE (expected_packet);
  for(size_t w=0; w<data->words; w++) {
    for(int i=0; i<4;i++){
      uint8_t byte = (data->buf[w]>>(i*8))&0xff;
      int index = w*4+i;
      CAPTURE (w, i, index, (int)byte);
      //CHECK (expected_packet.size() > index);
      if(expected_packet.size() > index) {
        auto exp = expected_packet[index];
        CAPTURE ((int)exp);
        CHECK ((int)byte == (int)exp);
      }
    } // i
  } // w
}
