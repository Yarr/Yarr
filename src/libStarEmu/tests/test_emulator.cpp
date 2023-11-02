#include "catch.hpp"
#include <deque>
#include <memory>
#include <fstream>

#include "LCBUtils.h"
#include "StarCmd.h"
#include "StarChipPacket.h"
#include "AllHwControllers.h"
#include "StarChips.h"
#include "EmuController.h"
#include "StarEmu.h"

void sendCommand(TxCore &hw, const std::array<uint16_t, 9> &cmd) {
  hw.writeFifo((LCB::IDLE << 16) + LCB::IDLE);
  hw.writeFifo((cmd[0] << 16) + cmd[1]);
  hw.writeFifo((cmd[2] << 16) + cmd[3]);
  hw.writeFifo((cmd[4] << 16) + cmd[5]);
  hw.writeFifo((cmd[6] << 16) + cmd[7]);
  hw.writeFifo((cmd[8] << 16) + LCB::IDLE);
}

void sendCommand(EmuTxCore<StarChips> &hw, uint32_t channel, const std::array<uint16_t, 9> &cmd) {
  hw.writeFifo(channel, (LCB::IDLE << 16) + LCB::IDLE);
  hw.writeFifo(channel, (cmd[0] << 16) + cmd[1]);
  hw.writeFifo(channel, (cmd[2] << 16) + cmd[3]);
  hw.writeFifo(channel, (cmd[4] << 16) + cmd[5]);
  hw.writeFifo(channel, (cmd[6] << 16) + cmd[7]);
  hw.writeFifo(channel, (cmd[8] << 16) + LCB::IDLE);
}

template<typename PacketT>
void compareOutputs(RawData* data, const PacketT& expected_packet);

template<typename PacketT>
void checkData(HwController*, std::map<uint32_t, std::deque<PacketT>>&, const PacketT *const mask_pattern = nullptr);

// Test by parsing bytes and comparing string
TEST_CASE("StarEmulatorParsing", "[star][emulator]") {
  std::shared_ptr<HwController> emu = StdDict::getHwController("emu_Star");

  REQUIRE (emu);

  json cfg = json::object();
  cfg["addressingModeDynamic"] = false;
  emu->loadConfig(cfg);

  StarCmd star;

  typedef std::string PacketCompare;

  // What data to expect, and how to mask the comparison
  std::map<uint32_t, std::deque<PacketCompare>> expected;

  SECTION("Read HCCStar interposed") {
    // Set LP_Enable to 1
    sendCommand(*emu, star.write_abc_register(32, 0x00000740));

    // read another HCCStar register
    std::array<LCB::Frame, 9> readHCCCmd2 = star.read_hcc_register(17);
    emu->writeFifo((readHCCCmd2[0] << 16) + readHCCCmd2[1]);
    // the read command is interupted by an L0A
    emu->writeFifo((LCB::l0a_mask(1, 0, false) << 16) + readHCCCmd2[2]);
    emu->writeFifo((readHCCCmd2[8] << 16) + LCB::IDLE);

    // Response from L0?
    expected[1].push_back("Packet type TYP_LP, BCID 7 (1), L0ID 3, nClusters 0\n");
    // NB this is incorrect?
    expected[1].push_back("Packet type TYP_HCC_RR, ABC 0, Address 11, Value 00000000\n");
  }

  SECTION("Read counter register") {
    // read an ABCStar register with broadcast addresses
    // Reading hit counter register
    std::array<LCB::Frame, 9> readABCCmd = star.read_abc_register(172);
    sendCommand(*emu, readABCCmd);

    expected[1].push_back("Packet type TYP_ABC_RR, ABC 0, Address ac, Value 00000000\n");
  }

  emu->releaseFifo();

  while(!emu->isCmdEmpty())
    ;

  checkData(emu.get(), expected);
}

TEST_CASE("StarEmulatorBytes", "[star][emulator]") {
  std::shared_ptr<HwController> emu = StdDict::getHwController("emu_Star");

  REQUIRE (emu);

  json cfg;
  emu->loadConfig(cfg);

  StarCmd star;

  typedef std::vector<uint8_t> PacketCompare;

  // What data to expect, and how to mask the comparison
  std::map<uint32_t, std::deque<PacketCompare>> expected;

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
  // ABC MaskHPR on, also set RRmode, LP Enable, and PR Enable to 1
  std::array<LCB::Frame, 9> writeABCCmd_MaskHPROn = star.write_abc_register(32, 0x00000740);
  sendCommand(*emu, writeABCCmd_MaskHPROn);
  // ABC StopHPR on
  std::array<LCB::Frame, 9> writeABCCmd_StopHPROn = star.write_abc_register(0, 0x00000004);
  sendCommand(*emu, writeABCCmd_StopHPROn);

  // Will still receive one initial HPR packet from HCC and one from each ABC
  // HCC HPR with Idle frame
  expected[1].push_back({0xe0, 0xf7, 0x85, 0x50, 0x02, 0xb0});
  // ABC HPR with Idle frame
  // (By default the emulator has only one hard-coded ABC with ID = 15 for now)
  expected[1].push_back({0xd0, 0x3f, 0x07, 0x85, 0x55, 0xff, 0xff, 0x00, 0x00});

  // Reset BC counters
  emu->writeFifo((LCB::IDLE << 16) + LCB::l0a_mask(0, 0, true));

  //////////////////////////
  // Start tests
  //////////////////////////

  SECTION("Read HCCStar") {
    // Read an HCCStar register
    std::array<LCB::Frame, 9> readHCCCmd = star.read_hcc_register(48); // 0x30
    sendCommand(*emu, readHCCCmd);

    // HCCStar register 48 (ADCcfg) is initialized to 0x00406600 
    expected[1].push_back({0x83, 0x00, 0x04, 0x06, 0x60, 0x00});
  }

  SECTION("Read HCCStar short") {
    // Read an HCCStar register using only 4 words
    std::array<LCB::Frame, 9> readHCCCmd = star.read_hcc_register(44); // 0x2c
    emu->writeFifo((readHCCCmd[0] << 16) + readHCCCmd[1]);
    emu->writeFifo((readHCCCmd[2] << 16) + readHCCCmd[8]);

    // HCCStar register 44 (Cfg2) is initialized to 0x0000018e
    expected[1].push_back({0x82, 0xc0, 0x00, 0x00, 0x18, 0xe0});
  }

  SECTION("Read ABCStar interposed") {
    // Write an ABCStar register first
    // (So its value is known here and does not depend on the default/reset)
    // Set register 34 (CREG2) to 0x00000190
    std::array<LCB::Frame, 9> writeABCCmd_reg = star.write_abc_register(34, 0x00000190);
    sendCommand(*emu, writeABCCmd_reg);
    // Read this register
    std::array<LCB::Frame, 9> readABCCmd =  star.read_abc_register(34); // 0x22
    emu->writeFifo((readABCCmd[0] << 16) + readABCCmd[1]);
    // The read command is interupted by an L0A
    emu->writeFifo((LCB::l0a_mask(1, 0, false) << 16) + readABCCmd[2]);
    emu->writeFifo((readABCCmd[8] << 16) + LCB::IDLE);

    // Response from L0A: empty cluster; l0tag = 0 + 3;
    // At the time of the L0A frame, bc count = 56; L0 latency is set to 400
    // => 8-bit BCID = (512 + 56-1 - 400) & 0xff = 0xa7
    // 4-bit BCID in the packet: 0b1111
    expected[1].push_back({0x20, 0x3f, 0x03, 0xfe, 0x6f, 0xed});
    // ABCStar register 34 (CREG2): 0x00000190
    expected[1].push_back({0x40, 0x22, 0x00, 0x00, 0x00, 0x19, 0x0f, 0x00, 0x00});
  }

  SECTION("Mask Registers") {
    // Switch to static test mode: TM = 1
    // (And MaskHPR = 1, LP_Enable = 1, PR_Enable = 1, RRMode = 1)
    std::array<LCB::Frame, 9> writeABCCmd_TM = star.write_abc_register(32, 0x00010740);
    sendCommand(*emu, writeABCCmd_TM);

    // Set mask registers
    std::array<LCB::Frame, 9> writeABCCmd_MaskInput3 = star.write_abc_register(19, 0xfffe0000);
    sendCommand(*emu, writeABCCmd_MaskInput3);
    std::array<LCB::Frame, 9> writeABCCmd_MaskInput7 = star.write_abc_register(23, 0xfffe0000);
    sendCommand(*emu, writeABCCmd_MaskInput7);

    // Send an L0A
    emu->writeFifo((LCB::IDLE << 16) + LCB::l0a_mask(1, 4, false));

    // l0tag = 4 + 3
    // bc count at L0A frame = 148; trigger command on 148-1; latency = 0
    // => 8-bit BCID = 0x93; 4-bit BCID in the packet = 0b0110
    expected[1].push_back({0x20, 0x76, 0x05, 0xc7, 0x01, 0xcf, 0x05, 0xe7, 0x01, 0xee, 0x07, 0xc7, 0x03, 0xcf, 0x07, 0xe7, 0x03, 0xee, 0x6f, 0xed});
  }

  SECTION("Hit Counters") {
    // Switch to static test mode (TM = 1) and enable hit counters
    // (And MaskHPR = 1, LP_Enable = 0, PR_Enable = 0, RRMode = 1)
    std::array<LCB::Frame, 9> writeABCCmd_TM = star.write_abc_register(32, 0x00010460);
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
      expected[1].push_back(mask_pattern);
    }

    // Check the hit counts
    // HitCountREG0
    std::array<LCB::Frame, 9> readABCCmd_hitcnt0 = star.read_abc_register(128);
    emu->writeFifo((readABCCmd_hitcnt0[0] << 16) + readABCCmd_hitcnt0[1]);
    emu->writeFifo((readABCCmd_hitcnt0[2] << 16) + readABCCmd_hitcnt0[8]);
    // HitCountREG0 for channel 0 to 3 is expected to be 0x04040404
    expected[1].push_back({0x40, 0x80, 0x00, 0x40, 0x40, 0x40, 0x4f, 0x00, 0x00});

    // HitCountREG63
    std::array<LCB::Frame, 9> readABCCmd_hitcnt63 = star.read_abc_register(191);
    emu->writeFifo((readABCCmd_hitcnt63[0] << 16) + readABCCmd_hitcnt63[1]);
    emu->writeFifo((readABCCmd_hitcnt63[2] << 16) + readABCCmd_hitcnt63[8]);
    // HitCountREG63 for channel 252 - 255 is expected be 0x00000000
    expected[1].push_back({0x40, 0xbf, 0x00, 0x00, 0x00, 0x00, 0x0f, 0x00, 0x00});
  }

  SECTION("LPEnable and EnCount") {
    // Set a mask register to non-zero value: MaskInput0 to 0x00000001
    sendCommand(*emu, star.write_abc_register(16, 0x00000001));

    // Configure ABCStar
    SECTION("Double Count") { // Both LP_Enable and EnCount are 1
      // MarkHPR = 1, RRMode = 1, TM = 1 (static test mode)
      // LP_Enable = 1, PR_Enable = 1, EnCount = 1
      sendCommand(*emu, star.write_abc_register(32, 0x00010760));

      // Expect an LP packet:
      // tag = 0+3;
      // BCID: at the time of L0A, bc count = 7; L0 latency = 0; so 8-bit BCID = 7 - 4 = 0x3. The 4-bit BCID in the packet = 0b0110;
      // clusters = {0x0000}: only the strip #0 has a hit
      // end of packet: 0x6fed
      expected[1].push_back({0x20, 0x36, 0x00, 0x00, 0x6f, 0xed});

      // Also expect value 0x00000001 from reading the hit counter register 0x80
      expected[1].push_back({0x40, 0x80, 0x00, 0x00, 0x00, 0x00, 0x1f, 0x00, 0x00});
    }

    SECTION("LP only") {
      // Enable LP, disable hit counters
      sendCommand(*emu, star.write_abc_register(32, 0x00010740));

      // Expect an LP packet with a hit at strip 0
      expected[1].push_back({0x20, 0x36, 0x00, 0x00, 0x6f, 0xed});

      // Register read from hit counter register 0x80 should have value 0
      expected[1].push_back({0x40, 0x80, 0x00, 0x00, 0x00, 0x00, 0x0f, 0x00, 0x00});
    }

    SECTION("Hit counts only") {
      // Disable LP, enable hit counters
      sendCommand(*emu, star.write_abc_register(32, 0x00010460));

      // No LP packets since LP_ENABLE is 0

      // Register read from hit counter register 0x80 should have value 1
      expected[1].push_back({0x40, 0x80, 0x00, 0x00, 0x00, 0x00, 0x1f, 0x00, 0x00});
    }

    // Reset and start hit counters
    emu->writeFifo((LCB::fast_command(LCB::ABC_HIT_COUNT_RESET, 0) << 16) + LCB::fast_command(LCB::ABC_HIT_COUNT_START, 0));

    // BC reset
    emu->writeFifo((LCB::IDLE << 16) + LCB::l0a_mask(0, 0, true));

    // Send an L0A and a register read command to read a hit count register
    emu->writeFifo((LCB::IDLE << 16) + LCB::l0a_mask(1, 0, false));
    sendCommand(*emu, star.read_abc_register(128));
  }

  SECTION("L0 Latency") {
    // Switch to test pulse mode: TM = 2, TestPulseEnable = 1
    // (And MaskHPR = 1, LP_Enable = 1, PR_Enable = 1, RRMode = 1)
    std::array<LCB::Frame, 9> writeABCCmd_cfg = star.write_abc_register(32, 0x00020750);
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

    // Physics packet: l0tag = 20 + 3
    // bc count = 156 when L0A command received; trigger on 156-1; latency = 3
    // 8-bit BCID = 152 (0x98) => 4-bit BCID in the packet = 0b0001
    expected[1].push_back({0x21, 0x71, 0x00, 0x00, 0x6f, 0xed});
  }

  emu->releaseFifo();

  while(!emu->isCmdEmpty())
    ;

  checkData(emu.get(), expected, &mask_pattern);
}

TEST_CASE("StarEmulatorHPR", "[star][emulator]") {
  std::shared_ptr<HwController> emu = StdDict::getHwController("emu_Star");

  REQUIRE (emu);

  json cfg;
  cfg["hprPeriod"] = 80; // Set HPR period to 80
  emu->loadConfig(cfg);

  StarCmd star;

  typedef std::vector<uint8_t> PacketCompare;

  std::map<uint32_t, std::deque<PacketCompare>> expected;

  // Send a logic reset first
  emu->writeFifo((LCB::IDLE << 16) + LCB::fast_command(LCB::LOGIC_RESET, 0));

  // Wait for the initial HPRs, which should arrive 80/2 BC after reset
  // Send Idle frames to keep the "clock" in the emulator running
  for (int j = 0; j < 6; j++) {
    emu->writeFifo((LCB::IDLE << 16) + LCB::IDLE);
  }

  // HCC HPR with Idle frame
  expected[1].push_back({0xe0, 0xf7, 0x85, 0x50, 0x02, 0xb0});
  // ABC HPR with Idle frame
  expected[1].push_back({0xd0, 0x3f, 0x07, 0x85, 0x55, 0xff, 0xff, 0x00, 0x00});

  SECTION("Periodic") {
    // Wait another 80 BCs for a second set of HPRs
    for (int j = 0; j < 10; j++) {
      emu->writeFifo((LCB::IDLE << 16) + LCB::IDLE);
    }

    expected[1].push_back({0xe0, 0xf7, 0x85, 0x50, 0x02, 0xb0});
    expected[1].push_back({0xd0, 0x3f, 0x07, 0x85, 0x55, 0xff, 0xff, 0x00, 0x00});
  }

  SECTION("StopHPR") {
    // Set HCC StopHPR bit to 1 to stop periodic HPR packets from HCCStar
    std::array<LCB::Frame, 9> writeHCCCmd_StopHPR = star.write_hcc_register(16, 0x00000001);
    sendCommand(*emu, writeHCCCmd_StopHPR);

    // Wait a bit, and we should only see a periodic HPR packet from ABCStar
    for (int j = 0; j < 4; j++) {
      emu->writeFifo((LCB::IDLE << 16) + LCB::IDLE);
    }

    expected[1].push_back({0xd0, 0x3f, 0x07, 0x85, 0x55, 0xff, 0xff, 0x00, 0x00});
  }

  SECTION("TestHPR") {
    // Set ABC TestHPR bit to 1 to receive an ABC HPR packet immediately
    std::array<LCB::Frame, 9> writeABCCmd_TestHPR = star.write_abc_register(0, 0x00000008);
    sendCommand(*emu, writeABCCmd_TestHPR);

    expected[1].push_back({0xd0, 0x3f, 0x07, 0x85, 0x55, 0xff, 0xff, 0x00, 0x00});

    // Wait a bit, and there should be the regular periodic HPR packets
    for (int j = 0; j < 4; j++) {
      emu->writeFifo((LCB::IDLE << 16) + LCB::IDLE);
    }

    expected[1].push_back({0xe0, 0xf7, 0x85, 0x50, 0x02, 0xb0});
    expected[1].push_back({0xd0, 0x3f, 0x07, 0x85, 0x55, 0xff, 0xff, 0x00, 0x00});
  }

  SECTION("MaskHPR") {
    // Set HCC MaskHPR bit to 1
    std::array<LCB::Frame, 9> writeHCCCmd_MaskHPR = star.write_hcc_register(43, 0x00000100);
    sendCommand(*emu, writeHCCCmd_MaskHPR);

    // Wait a bit for the periodic HPR packets
    for (int j = 0; j < 4; j++) {
      emu->writeFifo((LCB::IDLE << 16) + LCB::IDLE);
    }

    expected[1].push_back({0xe0, 0xf7, 0x85, 0x50, 0x02, 0xb0});
    expected[1].push_back({0xd0, 0x3f, 0x07, 0x85, 0x55, 0xff, 0xff, 0x00, 0x00});

    // Now set HCC TestHPR bit to 1.
    // No extra HCC HPR is expected since the HCC MaskHPR is on
    std::array<LCB::Frame, 9> writeHCCCmd_TestHPR = star.write_hcc_register(16, 0x00000002);
    sendCommand(*emu, writeHCCCmd_TestHPR);

    // Wait a bit, and there should still be periodic HPRs from both HCC and ABC
    for (int j = 0; j < 4; j++) {
      emu->writeFifo((LCB::IDLE << 16) + LCB::IDLE);
    }

    expected[1].push_back({0xe0, 0xf7, 0x85, 0x50, 0x02, 0xb0});
    expected[1].push_back({0xd0, 0x3f, 0x07, 0x85, 0x55, 0xff, 0xff, 0x00, 0x00});
  }

  emu->releaseFifo();

  while(!emu->isCmdEmpty());

  checkData(emu.get(), expected);
}

// Multiple star chips in one channel
TEST_CASE("StarEmulatorMultiChip", "[star][emulator]") {
  std::shared_ptr<HwController> emu = StdDict::getHwController("emu_Star");

  REQUIRE (emu);

  //////////////////////////
  // Set up configurations
  // Three HCCStars with ID 3, 4, 5
  // HCC 3 has two ABCs with ID 1 and 2
  json tmpRegCfg1;
  tmpRegCfg1["HCC"] = {{"ID", 3}};
  tmpRegCfg1["ABCs"] = {{"IDs", {1, 2}}};
  std::ofstream tmpRegFile1("test_star_1.json");
  tmpRegFile1 << std::setw(4) << tmpRegCfg1;
  tmpRegFile1.close();

  // HCC 4 has one ABC with ID 7
  json tmpRegCfg2;
  tmpRegCfg2["HCC"] = {{"ID", 4}};
  tmpRegCfg2["ABCs"] = {{"IDs", {7}}};
  std::ofstream tmpRegFile2("test_star_2.json");
  tmpRegFile2 << std::setw(4) << tmpRegCfg2;
  tmpRegFile2.close();

  // HCC 5 has three ABCs with ID 2, 4, 7
  json tmpRegCfg3;
  tmpRegCfg3["HCC"] = {{"ID", 5}};
  tmpRegCfg3["ABCs"] = {{"IDs", {2 ,4, 7}}};
  std::ofstream tmpRegFile3("test_star_3.json");
  tmpRegFile3 << std::setw(4) << tmpRegCfg3;
  tmpRegFile3.close();

  json tmpChipCfg;
  tmpChipCfg["chips"] = json::array();
  // Three front ends share the same tx channel 33
  // The first two FEs share the same rx channel 44
  int rx_fe1 = 44, rx_fe2 = 44;
  // The third one uses a separate rx channel 55
  int rx_fe3 = 55;
  tmpChipCfg["chips"][0] = {{"tx",33}, {"rx",rx_fe1}, {"config","test_star_1.json"}};
  tmpChipCfg["chips"][1] = {{"tx",33}, {"rx",rx_fe2}, {"config","test_star_2.json"}};
  tmpChipCfg["chips"][2] = {{"tx",33}, {"rx",rx_fe3}, {"config","test_star_3.json"}};
  std::string tmpChipFname("test_multichip_star_setup.json");
  std::ofstream tmpChipFile(tmpChipFname);
  tmpChipFile << std::setw(4) << tmpChipCfg;
  tmpChipFile.close();

  // EmuController config
  json cfg;
  cfg["chipCfg"] = tmpChipFname;
  cfg["addressingModeDynamic"] = false;
  emu->loadConfig(cfg);

  // Clean up
  remove(tmpChipFname.c_str());
  remove("test_star_1.json");
  remove("test_star_2.json");
  remove("test_star_3.json");

  StarCmd star;

  typedef std::pair<uint32_t, std::vector<uint8_t>> PacketCompare;
  std::map<uint32_t, std::deque<PacketCompare>> expected;

  SECTION("HCCStar Write and Read") {
    // Broadcast write command to set register 47 (ErrCfg) to 0xabcdabcd
    auto writeHCCCmd_reg47 = star.write_hcc_register(47, 0xabcdabcd);
    sendCommand(*emu, writeHCCCmd_reg47);

    // Read only the HCC with ID = 3
    auto readHCCCmd_3_reg47 = star.read_hcc_register(47, 3);
    sendCommand(*emu, readHCCCmd_3_reg47);
    expected[rx_fe1].push_back({rx_fe1, {0x82, 0xfa, 0xbc, 0xda, 0xbc, 0xd0}});

    // Write 0xdeadbeef to register 47 of the HCC with ID = 4
    auto writeHCCCmd_4_reg47 = star.write_hcc_register(47, 0xdeadbeef, 4);
    sendCommand(*emu, writeHCCCmd_4_reg47);

    // Write 0xcafebabe to register 47 of the HCC with ID = 5
    auto writeHCCCmd_5_reg47 = star.write_hcc_register(47, 0xcafebabe, 5);
    sendCommand(*emu, writeHCCCmd_5_reg47);

    // Broadcast read command to read register 47 of all HCCs
    auto readHCCCmd_47 = star.read_hcc_register(47);
    sendCommand(*emu, readHCCCmd_47);
    
    // HCC 3: register 47 is still 0xabcdabcd
    expected[rx_fe1].push_back({rx_fe1, {0x82, 0xfa, 0xbc, 0xda, 0xbc, 0xd0}});
    // HCC 4: register 47 should be 0xdeadbeef
    expected[rx_fe2].push_back({rx_fe2, {0x82, 0xfd, 0xea, 0xdb, 0xee, 0xf0}});
    // HCC 5: register 47 should be 0xcafebabe
    expected[rx_fe3].push_back({rx_fe3, {0x82, 0xfc, 0xaf, 0xeb, 0xab, 0xe0}});
  }

  SECTION("ABCStar Write and Read") {
    // Broadcast write 0xff7ff7ff to all MaskInput7 (addr=0x17) register
    auto writeABCCmd_mask7 = star.write_abc_register(0x17, 0xff7ff7ff, 0xf, 0xf);
    sendCommand(*emu, writeABCCmd_mask7);

    // Read MaskInput7 of all ABCs on HCC 3
    auto readABCCmd_hcc3 = star.read_abc_register(23, 3, 0xf);
    sendCommand(*emu, readABCCmd_hcc3);
    // Expect two ABC RR packets from ABC 1 and 2
    expected[rx_fe1].push_back({rx_fe1, {0x40, 0x17, 0x0f, 0xf7, 0xff, 0x7f, 0xf1, 0x00, 0x00}});
    expected[rx_fe1].push_back({rx_fe1, {0x41, 0x17, 0x0f, 0xf7, 0xff, 0x7f, 0xf2, 0x00, 0x00}});

    emu->releaseFifo();
    while(!emu->isCmdEmpty());
    checkData(emu.get(), expected);

    // Write 0xdeadbeef to MaskInput7 of ABC 7 on HCC 4
    auto writeABCCmd_4_7 = star.write_abc_register(0x17, 0xdeadbeef, 4, 7);
    sendCommand(*emu, writeABCCmd_4_7);

    // Write 0xaa0451aa to MaskInput7 of ABC 4 on HCC 5
    auto writeABCCmd_5_4 = star.write_abc_register(0x17, 0xaa0451aa, 5, 4);
    sendCommand(*emu, writeABCCmd_5_4);

    // Read MaskInput 7 of ABC 7 on HCC 5
    auto readABCCmd_5_7 = star.read_abc_register(0x17, 5, 7);
    sendCommand(*emu, readABCCmd_5_7);

    expected[rx_fe3].push_back({rx_fe3, {0x42, 0x17, 0x0f, 0xf7, 0xff, 0x7f, 0xf7, 0x00, 0x00}});

    emu->releaseFifo();
    while(!emu->isCmdEmpty());
    checkData(emu.get(), expected);

    // Broadcast read of MaskInput 7 of all ABCs
    auto readABCCmd_mask7 = star.read_abc_register(0x17, 0xf, 0xf);
    sendCommand(*emu, readABCCmd_mask7);

    // Two ABC RR packets from ABC 1 and 2 on HCC 3
    expected[rx_fe1].push_back({rx_fe1, {0x40, 0x17, 0x0f, 0xf7, 0xff, 0x7f, 0xf1, 0x00, 0x00}});
    expected[rx_fe1].push_back({rx_fe1, {0x41, 0x17, 0x0f, 0xf7, 0xff, 0x7f, 0xf2, 0x00, 0x00}});

    // One ABC RR packet from ABC 7 on HCC 4
    expected[rx_fe2].push_back({rx_fe2, {0x40, 0x17, 0x0d, 0xea, 0xdb, 0xee, 0xf7, 0x00, 0x00}});

    // Three ABC RR packet from ABC 2, 4, 7 on HCC 5
    expected[rx_fe3].push_back({rx_fe3, {0x40, 0x17, 0x0f, 0xf7, 0xff, 0x7f, 0xf2, 0x00, 0x00}});
    expected[rx_fe3].push_back({rx_fe3, {0x41, 0x17, 0x0a, 0xa0, 0x45, 0x1a, 0xa4, 0x00, 0x00}});
    expected[rx_fe3].push_back({rx_fe3, {0x42, 0x17, 0x0f, 0xf7, 0xff, 0x7f, 0xf7, 0x00, 0x00}});
  }

  emu->releaseFifo();
  while(!emu->isCmdEmpty());
  checkData(emu.get(), expected);
}

// Multiple channels
TEST_CASE("StarEmuLatorMultiChannel", "[star][emulator]") {
  std::shared_ptr<HwController> emu = StdDict::getHwController("emu_Star");

  REQUIRE (emu);

  //////////////////////////
  // Create a chip configuration
  json tmpChipCfg;
  tmpChipCfg["chips"] = json::array();

  // Three channels, each channel contains one HCCStar chip
  // Each HCCStar is configured to have one ABCStar if no other config provided
  unsigned nchannels = 3;
  std::string tmpFileName("test_multichannel_star_setup.json");
  for (unsigned i=0; i<nchannels; i++) {
    tmpChipCfg["chips"][i] = {{"tx", 2*i}, {"rx", 2*i+1}};
  }
  std::ofstream tmpCfgFile(tmpFileName);
  tmpCfgFile << std::setw(4) << tmpChipCfg;
  tmpCfgFile.close();

  //////////////////////////
  // Emulator configuration
  json cfg;
  cfg["chipCfg"] = tmpFileName;

  emu->loadConfig(cfg);

  remove(tmpFileName.c_str());

  StarCmd star;

  typedef std::pair<uint32_t, std::vector<uint8_t>> PacketCompare;

  std::map<uint32_t, std::deque<PacketCompare>> expected;

  //////////////////////////
  // Initialization
  // First turn off both HCC and ABC HPRs i.e. MaskHPR=1, StopHPR=1
  // Commands are broacasted to all HCCStar and ABCStar chips
  auto writeHCCCmd_MaskHPR = star.write_hcc_register(43, 0x00000100);
  sendCommand(*emu, writeHCCCmd_MaskHPR);
  auto writeHCCCmd_StopHPR = star.write_hcc_register(16, 0x00000001);
  sendCommand(*emu, writeHCCCmd_StopHPR);
  auto writeABCCmd_MaskHPR = star.write_abc_register(32, 0x00000740); // Also LPEnable = 1, PREnable = 1, RRmode = 1
  sendCommand(*emu, writeABCCmd_MaskHPR);
  auto writeABCCmd_StopHPR = star.write_abc_register(0, 0x00000004);
  sendCommand(*emu, writeABCCmd_StopHPR);

  // Expected to receive one initial HPR packet from HCCStar and one from ABCStar
  for (unsigned i=0; i<nchannels; i++) {
    uint32_t chn_rx = 2*i+1;
    // hcc hpr with idle frame
    expected[chn_rx].push_back({chn_rx, {0xe0, 0xf7, 0x85, 0x50, 0x02, 0xb0}});
  }

  for (unsigned i=0; i<nchannels; i++) {
    uint32_t chn_rx = 2*i+1;
    // abc hpr with idle frame
    expected[chn_rx].push_back({chn_rx, {0xd0, 0x3f, 0x07, 0x85, 0x55, 0xff, 0xff, 0x00, 0x00}});
  }

  emu->releaseFifo();
  while(!emu->isCmdEmpty());
  checkData(emu.get(), expected);

  //////////////////////////
  // Start tests

  SECTION("Register read from all channels") {
    // Read HCCStar registers ADCcfg
    auto readHCCCmd_ADCcfg = star.read_hcc_register(48);
    // Broadcast command
    sendCommand(*emu, readHCCCmd_ADCcfg);

    // Expect to read back the default value 0x00406600 from all HCCStars
    for (unsigned i=0; i<nchannels; i++) {
      uint32_t chn_rx = 2*i+1;
      expected[chn_rx].push_back({chn_rx, {0x83, 0x00, 0x04, 0x06, 0x60, 0x00}});
    }

    emu->releaseFifo();
    while(!emu->isCmdEmpty());
    checkData(emu.get(), expected);
  }

  SECTION("Rx channel control") {
    // Write all MaskInput7 of ABCStar chips
    auto writeABCCmd_mask7 = star.write_abc_register(23, 0xdeadbeef);
    sendCommand(*emu, writeABCCmd_mask7);

    // Write all MaskInput6 of ABCStar chips
    auto writeABCCmd_mask6 = star.write_abc_register(22, 0xcafebabe);
    sendCommand(*emu, writeABCCmd_mask6);

    // Disable all rx channels except for rx channel 3
    emu->disableRx();
    emu->setRxEnable(3);

    // Send a broadcast register read command to read MaskInput7
    auto readABCCmd_mask7 = star.read_abc_register(23);
    sendCommand(*emu, readABCCmd_mask7);

    // Expect to receive data only from rx channel 3
    expected[3].push_back({3, {0x40, 0x17, 0x0d, 0xea, 0xdb, 0xee, 0xff, 0x00, 0x00}});

    emu->releaseFifo();
    while(!emu->isCmdEmpty());
    checkData(emu.get(), expected);

    // Now disable all rx channels except for rx channel 1
    emu->disableRx();
    emu->setRxEnable(1);

    // Send a broadcast register read command to read MaskInput6
    auto readABCCmd_mask6 = star.read_abc_register(22);
    sendCommand(*emu, readABCCmd_mask6);

    // Should receive data only from rx channel 1.
    // Two data packets are expected:
    // One is from the previous MaskInput7 read command: the register read packet was pushed to the rx buffer but had not been read out since the rx channel was disabled.
    expected[1].push_back({1, {0x40, 0x17, 0x0d, 0xea, 0xdb, 0xee, 0xff, 0x00, 0x00}});
    // The second packet is from the MaskInput6 read command.
    expected[1].push_back({1, {0x40, 0x16, 0x0c, 0xaf, 0xeb, 0xab, 0xef, 0x00, 0x00}});

    emu->releaseFifo();
    while(!emu->isCmdEmpty());
    checkData(emu.get(), expected);
  }

  SECTION("Tx command control") {
    // Enable tx channel 2, disable all others
    emu->disableCmd();
    emu->setCmdEnable(2);

    // Broadcast a register write command to change the value of MaskInput0
    auto writeABCCmd_mask0 = star.write_abc_register(16, 0xdecafbad);
    sendCommand(*emu, writeABCCmd_mask0);

    // Read all MaskInput0 registers
    // Enable all tx channels first
    std::vector<uint32_t> enabled_tx;
    for (unsigned i=0; i<nchannels; i++) {
      unsigned chn_tx = 2*i;
      enabled_tx.push_back(chn_tx);
    }
    emu->setCmdEnable(enabled_tx);
    auto readABCCmd_mask0 = star.read_abc_register(16); // broadcast reg read
    sendCommand(*emu, readABCCmd_mask0);

    // Only the ABCStar on tx channel 2 is expected to have its value updated
    // Others should still have the default value 0x0
    for (unsigned i=0; i<nchannels; i++) {
      unsigned chn_tx = 2*i;
      unsigned chn_rx = 2*i+1;
      if (chn_tx == 2)
        expected[chn_rx].push_back({chn_rx, {0x40, 0x10, 0x0d, 0xec, 0xaf, 0xba, 0xdf, 0x00, 0x00}});
      else
        expected[chn_rx].push_back({chn_rx, {0x40, 0x10, 0x00, 0x00, 0x00, 0x00, 0x0f, 0x00, 0x00}});
    }

    emu->releaseFifo();
    while(!emu->isCmdEmpty());
    checkData(emu.get(), expected);
  }

  SECTION("Tx trigger control") {
    // Reset BC counters
    emu->writeFifo((LCB::IDLE << 16) + LCB::l0a_mask(0, 0, true));

    // Switch to the static test mode (TM = 1)
    // (And MaskHPR = 1, LP_Enable = 1, PR_Enable = 1, RRMode = 1)
    auto writeABCCmd_TM = star.write_abc_register(32, 0x00010740);
    sendCommand(*emu, writeABCCmd_TM);

    // Write 0xfffe0000 to MaskInput3 so we will have a non-empty cluster
    auto writeABCCmd_mask3 = star.write_abc_register(19, 0xfffe0000);
    sendCommand(*emu, writeABCCmd_mask3);

    // Prepare trigger
    // Trigger counts
    emu->setTrigCnt(2);
    // Trigger words
    std::array<uint32_t, 3> trigWord;
    trigWord[0] = (LCB::IDLE << 16) + LCB::IDLE;
    trigWord[1] = (LCB::IDLE << 16) + LCB::IDLE;
    trigWord[2] = (LCB::l0a_mask(1, 4, false) << 16) | LCB::IDLE;
    emu->setTrigWord(&trigWord[0], 3);

    // Disable trigger commands in all channels except for tx channel 2 and 4
    emu->disableCmd();
    emu->setCmdEnable({2, 4});

    while(!emu->isCmdEmpty());

    // Start trigger
    emu->setTrigEnable(1);

    // Cluster bytes from MaskInput3 value 0xfffe0000:
    // {0x05,0xc7, 0x01,0xcf, 0x05,0xe7, 0x01,0xee}
    // L0tag from the trigger words: l0tag = 4 (input tag) + 3 (BC offset)
    // And two packets from rx channel 5 (corresponding to tx 4)
    // Expect two physics packets from rx channel 3 (corresponding to tx 2)
    expected[3].push_back({3, {0x20,0x7e, 0x05,0xc7, 0x01,0xcf, 0x05,0xe7, 0x01,0xee, 0x6f,0xed}}); // bcid = 0b1110
    expected[5].push_back({5, {0x20,0x7e, 0x05,0xc7, 0x01,0xcf, 0x05,0xe7, 0x01,0xee, 0x6f,0xed}}); // bcid = 0b1110
    expected[3].push_back({3, {0x20,0x7e, 0x05,0xc7, 0x01,0xcf, 0x05,0xe7, 0x01,0xee, 0x6f,0xed}}); // bcid = 0b1110
    expected[5].push_back({5, {0x20,0x7e, 0x05,0xc7, 0x01,0xcf, 0x05,0xe7, 0x01,0xee, 0x6f,0xed}}); // bcid = 0b1110

    // Join trigger process when it is done
    while(!emu->isTrigDone());
    emu->setTrigEnable(0);

    checkData(emu.get(), expected);
  }
}

// Multi-level trigger mode
TEST_CASE("StarEmulatorR3L1", "[star][emulator]") {
  std::shared_ptr<HwController> emu = StdDict::getHwController("emu_Star");
  // Downcast to EmuController in order to write to a specific channel using writeFifo(uint32_t chn, uint32_t value)
  auto staremu = std::dynamic_pointer_cast<EmuController<StarChips,StarEmu>>(emu);

  REQUIRE (staremu);

  // Create a chip configuration
  json tmpChipCfg;
  tmpChipCfg["chips"] = json::array();
  // One chip (1 HCCStar + 1 ABCStar) with two tx channels and one rx channel
  // tx uses channel #0; tx2 uses channel #2
  tmpChipCfg["chips"][0] = {{"tx", 0}, {"tx2", 2}, {"rx", 1}};

  // Save the chip config file to disk
  std::string tmpFileName("test_staremu_r3l1.json");
  std::ofstream tmpCfgFile(tmpFileName);
  tmpCfgFile << std::setw(4) << tmpChipCfg;
  tmpCfgFile.close();

  // Load emulator configuration
  json cfg;
  cfg["type"] = "emu_Star";
  cfg["cfg"]  = json::object();
  cfg["cfg"]["chipCfg"] = tmpFileName;
  cfg["cfg"]["addressingModeDynamic"] = false;

  staremu->loadConfig(cfg);

  remove(tmpFileName.c_str());

  StarCmd star;

  typedef std::vector<uint8_t> PacketCompare;
  std::map<uint32_t, std::deque<PacketCompare>> expected;

  //////////////////////////
  // Initialize the emulator
  // Turn off HPRs: MaskHPR=1, StopHPR=1
  // LCB command sent via tx channel 0
  // Send IDLE packets of equal length via tx2 channel 2 to ensure the channels are in sync
  std::array<uint16_t, 9> IdleCmd;
  for (size_t i=0; i<9; i++) {
    IdleCmd[i] =  LCB::IDLE;
  }

  // tx (channel 0)
  auto writeHCCCmd_MaskHPR = star.write_hcc_register(43, 0x00000100);
  sendCommand(*staremu, 0, writeHCCCmd_MaskHPR);
  // tx2 (channel 2)
  sendCommand(*staremu, 2, IdleCmd);

  // tx (channel 0)
  auto writeHCCCmd_StopHPR = star.write_hcc_register(16, 0x00000001);
  sendCommand(*staremu, 0, writeHCCCmd_StopHPR);
  // tx2 (channel 2)
  sendCommand(*staremu, 2, IdleCmd);

  // tx (channel 0)
  // MaskHPR = 1, LP_Enable = 1, PR_Enable = 1, RRMode = 1
  auto writeABCCmd_MaskHPR = star.write_abc_register(32, 0x00000740);
  sendCommand(*staremu, 0, writeABCCmd_MaskHPR);
  // tx2 (channel 2)
  sendCommand(*staremu, 2, IdleCmd);

  // tx (channel 0)
  auto writeABCCmd_StopHPR = star.write_abc_register(0, 0x00000004);
  sendCommand(*staremu, 0, writeABCCmd_StopHPR);
  // tx2 (channel 2)
  sendCommand(*staremu, 2, IdleCmd);

  // Expect to receive initial HPR packets
  // HCC HPR with Idle frame
  expected[1].push_back({0xe0, 0xf7, 0x85, 0x50, 0x02, 0xb0});
  // ABC HPR with Idle frame
  expected[1].push_back({0xd0, 0x3f, 0x07, 0x85, 0x55, 0xff, 0xff, 0x00, 0x00});

  // Switch to multi-level trigger mode
  // Register 41 OPmode bit 0
  auto writeHCCCmd_trigmode = star.write_hcc_register(41, 0x00020000);
  sendCommand(*staremu, 0, writeHCCCmd_trigmode);
  sendCommand(*staremu, 2, IdleCmd);
  // Register 42 OPmodeC as well?
  //auto writeHCCCmd_trigmodec = star.write_hcc_register(42, 0x00020000);
  //sendCommand(*staremu, 0, writeHCCCmd_trigmodec);
  //sendCommand(*staremu, 2, IdleCmd);

  //////////////////////////
  // Start test
  // Set MaskInput0 register to 0x00000001 so there would be a non-empty cluster
  auto writeABCCmd_mask = star.write_abc_register(16, 0x00000001);
  sendCommand(*staremu, 0, writeABCCmd_mask);
  sendCommand(*staremu, 2, IdleCmd);

  // Set L0 latency to e.g. 5
  auto writeABCCmd_lat = star.write_abc_register(34, 0x00000005);
  sendCommand(*staremu, 0, writeABCCmd_lat);
  sendCommand(*staremu, 2, IdleCmd);

  // Set the the HCC ID to e.g. 10 so it would respond to an R3 command
  // (default serial number from HCC register 17 is 0 in the emulator)
  uint8_t hccID = 10; // module #5
  auto writeHCCCmd_id = star.write_hcc_register(17, hccID<<24);
  sendCommand(*staremu, 0, writeHCCCmd_id);
  sendCommand(*staremu, 2, IdleCmd);

  SECTION("LPEnable=1 PREnable=1") {
    // Switch to test pulse mode: TM = 2, TestPulseEnable = 1
    // MaskHPR = 1, LP_Enable = 1, PR_Enable = 1, RRMode = 1
    sendCommand(*staremu, 0, star.write_abc_register(32, 0x00020750));
    sendCommand(*staremu, 2, IdleCmd);

    // Expected packets from the test sequence below
    // First a PR packet: tag = 42 (0x2a); 4-bit BCID = 0b0110
    expected[1].push_back({0x12, 0xa6, 0x00, 0x00, 0x6f, 0xed});

    // A second PR packet: tag = 66 (0x42), 4-bit BCID = 0b1111
    expected[1].push_back({0x14, 0x2f, 0x00, 0x00, 0x6f, 0xed});

    // Followed by an LP packet with tag = 42 (0x2a), 4-bit BCID = 0b0110
    expected[1].push_back({0x22, 0xa6, 0x00, 0x00, 0x6f, 0xed});
  }

  SECTION("LPEnable=0 PREnable=1") {
    // Switch to test pulse mode: TM = 2, TestPulseEnable = 1
    // MaskHPR = 1, LP_Enable = 0, PR_Enable = 1, RRMode = 1
    sendCommand(*staremu, 0, star.write_abc_register(32, 0x00020550));
    sendCommand(*staremu, 2, IdleCmd);

    // Expected packets from the test sequence below
    // First a PR packet: tag = 42 (0x2a); 4-bit BCID = 0b0110
    expected[1].push_back({0x12, 0xa6, 0x00, 0x00, 0x6f, 0xed});

    // A second PR packet: tag = 66 (0x42), 4-bit BCID = 0b1111
    expected[1].push_back({0x14, 0x2f, 0x00, 0x00, 0x6f, 0xed});

    // No more LP packet since LP_ENABLE is 0
  }

  SECTION("LPEnable=1 PREnable=0") {
    // Switch to test pulse mode: TM = 2, TestPulseEnable = 1
    // MaskHPR = 1, LP_Enable = 1, PR_Enable = 0, RRMode = 1
    sendCommand(*staremu, 0, star.write_abc_register(32, 0x00020650));
    sendCommand(*staremu, 2, IdleCmd);

    // No PR packets since PR_ENABLE is 0

    // Expect an LP packet with tag = 42 (0x2a), 4-bit BCID = 0b0110
    expected[1].push_back({0x22, 0xa6, 0x00, 0x00, 0x6f, 0xed});
  }

  SECTION("LPEnable=0 PREnable=0") {
    // Switch to test pulse mode: TM = 2, TestPulseEnable = 1
    // MaskHPR = 1, LP_Enable = 0, PR_Enable = 0, RRMode = 1
    sendCommand(*staremu, 0, star.write_abc_register(32, 0x00020450));
    sendCommand(*staremu, 2, IdleCmd);

    // Expect no PR packets nor LP packets
  }

  // Reset BC counters
  staremu->writeFifo(0, (LCB::IDLE << 16) + LCB::l0a_mask(0, 0, true));
  staremu->writeFifo(2, (LCB::IDLE << 16) + LCB::IDLE);

  // Send two digital pulse commands
  staremu->writeFifo(0, (LCB::fast_command(LCB::ABC_DIGITAL_PULSE, 3) << 16) + LCB::fast_command(LCB::ABC_DIGITAL_PULSE, 3));
  staremu->writeFifo(2, (LCB::IDLE << 16) + LCB::IDLE);

  // Send an L0A command with L0tag = 42 that triggers on the first pulse
  unsigned tag1 = 42;
  staremu->writeFifo(0, (LCB::IDLE << 16) + LCB::l0a_mask(8, tag1, false));
  staremu->writeFifo(2, (LCB::IDLE << 16) + LCB::IDLE);

  // Send a second L0A command with L0tag = 66 that triggers on the second pulse
  unsigned tag2 = 66;
  staremu->writeFifo(0, (LCB::l0a_mask(8, tag2, false) << 16) + LCB::IDLE);
  // At the same time, send an R3 to read the first pulse with l0tag 42
  unsigned modulemask = 1<<4;
  // encode into 16b frame
  uint16_t r3frame1 = LCB::raw_bits( (modulemask<<7) + tag1 );
  staremu->writeFifo(2, (r3frame1 << 16) + LCB::IDLE);

  // Send another R3 frame to read the second event with tag 66
  // Followed by an L1 command that read the previous event
  staremu->writeFifo(0, (LCB::IDLE << 16) + LCB::IDLE);
  uint16_t r3frame2 = LCB::raw_bits( (modulemask<<7) + tag2 );
  uint16_t l1frame = LCB::raw_bits( tag1 );
  staremu->writeFifo(2, (r3frame2 << 16) + l1frame);

  staremu->releaseFifo();
  while(!staremu->isCmdEmpty());
  checkData(staremu.get(), expected);
}

TEST_CASE("StarEmulatorVersions", "[star][emulator]") {
  std::shared_ptr<HwController> emu = StdDict::getHwController("emu_Star");

  REQUIRE (emu);

  json cfg;

  SECTION("Default") {}

  SECTION("PPA") {
    cfg["abcVersion"] = 1;
  }

  SECTION("PPB") {
    cfg["abcVersion"] = 1;
    cfg["hccVersion"] = 1;
  }

  emu->loadConfig(cfg);

  // Send reset fast commands (these include loops over register lists)
  emu->writeFifo((LCB::IDLE << 16) + LCB::fast_command(LCB::LOGIC_RESET, 0));
  emu->writeFifo((LCB::IDLE << 16) + LCB::fast_command(LCB::ABC_REG_RESET, 0));
  emu->writeFifo((LCB::IDLE << 16) + LCB::fast_command(LCB::HCC_REG_RESET, 0));

  StarCmd star;

  typedef std::string PacketCompare;

  // What data to expect, and how to mask the comparison
  std::map<uint32_t, std::deque<PacketCompare>> expected;

  // For now just read a generic register
  auto readABCCmd_mask0 = star.read_abc_register(16); // broadcast reg read
  sendCommand(*emu, readABCCmd_mask0);

  expected[1].push_back("Packet type TYP_ABC_RR, ABC 0, Address 10, Value 00000000\n");

  emu->releaseFifo();

  while(!emu->isCmdEmpty())
    ;

  checkData(emu.get(), expected);
}

template<typename PacketT>
void checkData(HwController* emu, std::map<uint32_t, std::deque<PacketT>>& expected, const PacketT *const mask_pattern)
{
  std::vector<RawDataPtr> dataVec;

  for(int reads=0; reads<10; reads++) {
      CAPTURE (reads);
      dataVec = emu->readData();
      CAPTURE(dataVec.size());
      for (unsigned i=0; i<dataVec.size(); i++) {
          auto data = dataVec[i];
          if(data) {
              CAPTURE (data->getSize());
              CAPTURE (data->getAdr());
              unsigned channel = data->getAdr();
              CHECK (!expected[channel].empty());

              PacketT expected_packet;
              if (!expected[channel].empty()) {
                  expected_packet = expected[channel].front();
                  expected[channel].pop_front();
              }

              CAPTURE(expected[channel].size());

              // Do comparison
              if (not mask_pattern) {
                  compareOutputs(data.get(), expected_packet);
              } else if (expected_packet != (*mask_pattern)) {
                  compareOutputs(data.get(), expected_packet);
              }

          }

      }
  }

  // TODO need to check all entries in the map
  //CHECK(expected.empty());
}

template<>
void compareOutputs<std::string>(RawData* data, const std::string& expected_packet)
{
  StarChipPacket packet;
  packet.add_word(0x13c); //add SOP
  for(unsigned iw=0; iw<data->getSize(); iw++) {
    for (int i=0; i<4;i++){
      packet.add_word((data->get(iw)>>i*8)&0xff);
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
  for(size_t w=0; w<data->getSize(); w++) {
    for(int i=0; i<4;i++){
      uint8_t byte = (data->get(w)>>(i*8))&0xff;
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

template<>
void compareOutputs<std::pair<uint32_t, std::vector<uint8_t>>>(RawData* data, const std::pair<uint32_t, std::vector<uint8_t>>& expected)
{
  // Compare channel numbers
  uint32_t rx_exp = expected.first;
  CAPTURE (rx_exp);
  uint32_t rx_data = data->getAdr();
  CAPTURE (rx_data);

  CHECK (rx_exp == rx_data);

  // Compare the actual data
  compareOutputs<std::vector<uint8_t>>(data, expected.second);
}
