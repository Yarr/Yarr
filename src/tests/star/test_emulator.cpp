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

  // Starts with an HPR
  expected.push_back("Packet type TYP_ABC_HPR, ABC 0, Address 3f, Value 78555fff\n");

  // This is the frame that goes in the initial HPR
  emu->writeFifo((LCB::IDLE << 16) + LCB::IDLE);

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

  std::unique_ptr<RawData> data(emu->readData());

  for(int reads=0; reads<10; reads++) {
    CAPTURE (reads);
    if(data) {
      CHECK (!expected.empty());

      PacketCompare expected_packet;
      if(!expected.empty()) {
        expected_packet = expected.front();
        expected.pop_front();
      }

      CAPTURE (data->words);

      StarChipPacket packet;
      packet.add_word(0x13c); //add SOP
      for(unsigned iw=0; iw<data->words; iw++) {
        for(int i=0; i<4;i++){
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

    data.reset(emu->readData());
  }

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

  // Starts with an HPR
  expected.push_back({0xd0, 0x3f, 0x07, 0x85, 0x55, 0xff, 0xf0, 0x00});

  SECTION("Counters") {
    // Send fast commands:
    emu->writeFifo((LCB::IDLE << 16) + LCB::fast_command(LCB::ABC_HIT_COUNT_RESET, 0));
    emu->writeFifo((LCB::IDLE << 16) + LCB::fast_command(LCB::ABC_HITCOUNT_START, 0));
    emu->writeFifo((LCB::IDLE << 16) + LCB::IDLE);

    // Write to ABCStar register CREG0 to enable hit counters
    std::array<LCB::Frame, 9> writeABCCmd = star.write_abc_register(32, 0x00000020); 
    sendCommand(*emu, writeABCCmd);

    // send an L0A
    emu->writeFifo((LCB::IDLE << 16) + LCB::l0a_mask(10, 0, false));

    expected.push_back({
        0x20, 0x00, 0x07, 0x8f, 0x03, 0x8f, 0x07, 0xaf,
        0x03, 0xaf, 0x07, 0xcf, 0x03, 0xcf, 0x07, 0xee,
        0x03, 0xee, 0x6f, 0xed, 0xff});

    expected.push_back({
        0x20, 0x20, 0x07, 0x8f, 0x03, 0x8f, 0x07, 0xaf,
        0x03, 0xaf, 0x07, 0xcf, 0x03, 0xcf, 0x07, 0xee,
        0x03, 0xee, 0x6f, 0xed, 0xff});
  }

  SECTION("Read HCCStar") {
    // read an HCCStar register
    //
    std::array<LCB::Frame, 9> readHCCCmd = star.read_hcc_register(44);
    sendCommand(*emu, readHCCCmd);

    expected.push_back({0x82, 0xc0, 0x00, 0x00, 0x18, 0xe0, 0x00, 0x00, 0xff}); 
  }

  SECTION("Read HCCStar short") {
    // read an HCCStar register using only 4 words
    std::array<LCB::Frame, 9> readHCCCmd = star.read_hcc_register(44);
    emu->writeFifo((readHCCCmd[0] << 16) + readHCCCmd[1]);
    emu->writeFifo((readHCCCmd[2] << 16) + readHCCCmd[8]);

    expected.clear();
    // As there are no initial idles the initial HPR is different
    expected.push_back({0xd0, 0x3f, 0x04, 0x75, 0xc5, 0xff, 0xf0, 0x00});

    expected.push_back({0x82, 0xc0, 0x00, 0x00, 0x18, 0xe0, 0x00, 0x00, 0xff});
  }

  emu->releaseFifo();

  while(!emu->isCmdEmpty())
    ;

  std::unique_ptr<RawData> data(emu->readData());

  for(int reads=0; reads<10; reads++) {
    CAPTURE (reads);
    if(data) {
      CHECK (!expected.empty());

      PacketCompare expected_packet;
      if(!expected.empty()) {
        expected_packet = expected.front();
        expected.pop_front();
      }

      CAPTURE (data->words);

      auto bytes = expected_packet;
      CAPTURE (bytes);
      for(size_t w=0; w<data->words; w++) {
        for(int i=0; i<4;i++){
          uint8_t byte = (data->buf[w]>>(i*8))&0xff;
          int index = w*4+i;
          CAPTURE (w, i, index, (int)byte);
          CHECK (bytes.size() > index);
          if(bytes.size() > index) {
            auto exp = bytes[index];
            CAPTURE ((int)exp);
            CHECK ((int)byte == (int)exp);
          }
        }
      }
    }

    data.reset(emu->readData());
  }

  emu->setRxEnable(0x0);
}
