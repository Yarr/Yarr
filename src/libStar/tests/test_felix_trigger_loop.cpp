#include "catch.hpp"
#include <memory>

#include "AllStdActions.h"
#include "EmptyHw.h"
#include "LCBFwUtils.h"
#include "StarFelixTriggerLoop.h"

/*
  TxCore to record bytes that are sent to different tx channels
*/
class TestFwTxCore : public EmptyTxCore {

public:

  void setCmdEnable(uint32_t elink) override {
    // disable all e-links
    for (auto& el : m_enables) {
      m_enables[el.first] = false;
    }

    m_enables[elink] = true;
    m_fifo[elink];
  }

  void setCmdEnable(std::vector<uint32_t> elinks) override {
    // disable all e-links first
    for (auto& el : m_enables) {
      m_enables[el.first] = false;
    }

    // enable the specified e-links
    for (const auto& chn : elinks) {
      m_enables[chn] = true;
      m_fifo[chn];
    }
  }

  void writeFifo(uint32_t word) override {
    for (auto& [elink, buf] : m_fifo) {
      if (not m_enables[elink]) continue;

      // Split 32b word into 4 bytes. MSB first
      buf.push_back( (word>>24) & 0xff );
      buf.push_back( (word>>16) & 0xff );
      buf.push_back( (word>>8) & 0xff );
      buf.push_back( word & 0xff );
    }
  }

  void setTrigWord(uint32_t *word, uint32_t length) override {
    m_trigWords.clear();
    for(int i=0; i<length; i++) {
      m_trigWords.push_back(word[i]);
    }
  }

  void setTrigWordLength(uint32_t length) override {
    m_trigWordLength = length;
  }

  void setTrigCnt(uint32_t count) override {
    m_nPulse = count;
  }

  std::map<uint32_t, bool> m_enables; // enable flags for elinks
  std::map<uint32_t, std::vector<uint8_t>> m_fifo; // buffers for elinks

  std::vector<uint32_t> m_trigWords;
  uint32_t m_trigWordLength;
  uint32_t m_nPulse;
};

/*
  Some helper functions for checking triggers stored in the buffer / trickle memory
*/

uint32_t count_triggers(const std::vector<uint8_t>& buffer) {
  uint32_t ntrigs = 0;

  uint32_t ibyte = 0;
  while (ibyte < buffer.size()) {
    if (buffer[ibyte] == LCB_FELIX::L0A) {
      // found an L0A frame
      // check the lowest 4 bits of the next byte for L0A mask
      if (buffer[ibyte+1] & 8) ntrigs+=1;
      if (buffer[ibyte+1] & 4) ntrigs+=1;
      if (buffer[ibyte+1] & 2) ntrigs+=1;
      if (buffer[ibyte+1] & 1) ntrigs+=1;
      ibyte += 2;
    } else if (buffer[ibyte] == LCB_FELIX::FASTCMD) {
      ibyte += 2;
    } else if (buffer[ibyte] == LCB_FELIX::HCCREGRD or buffer[ibyte] == LCB_FELIX::ABCREGRD) {
      ibyte += 3;
    } else if (buffer[ibyte] == LCB_FELIX::HCCREGWR or buffer[ibyte] == LCB_FELIX::ABCREGWR) {
      ibyte += 7;
    } else { // IDLE
      ibyte += 1;
    }
  }

  return ntrigs;
}

double get_trig_frequency(const std::vector<uint8_t>& buffer) {

  uint32_t BC = 0; // bunch crossing counter
  std::vector<uint32_t> trigBCs; // BC of triggers

  uint32_t ibyte = 0;
  while (ibyte < buffer.size()) {

    if (buffer[ibyte] == LCB_FELIX::L0A) { // an L0A frame
      // Check the next byte for trigger mask
      uint8_t mask = buffer[ibyte+1] & 0xf; // get the lowest 4 bits

      // Check each bit of mask. The highest bit corresponds to the earliest BC.
      for (unsigned s = 0; s < 4; s++) {
        if ( mask & (1<<(3-s)) ) {
          // There is a trigger
          trigBCs.push_back(BC);
        }

        BC += 1;
      }
      ibyte += 2;

    } else if (buffer[ibyte] == LCB_FELIX::IDLE) { // an IDLE frame
      BC += 4; // 4 BCs per frame
      ibyte += 1;

    } else if (buffer[ibyte] == LCB_FELIX::FASTCMD) { // a fast command
      BC += 4; // 4 BCs per frame
      ibyte += 2;

    } else if (buffer[ibyte] == LCB_FELIX::HCCREGRD or buffer[ibyte] == LCB_FELIX::ABCREGRD) {
      BC += 4*4; // 4 LCB frames, 4 BCs per frame
      ibyte += 3;

    } else if (buffer[ibyte] == LCB_FELIX::HCCREGWR or buffer[ibyte] == LCB_FELIX::ABCREGWR) {
      BC += 9*4; // 9 LCB frames, 4 BCs per frame
      ibyte += 7;

    } else {
      // Shouldn't be here. Just in case
      ibyte += 1;
    }

    if (trigBCs.size() > 2)
      break;

  } // end of while (ibyte < buffer.size())

  if (trigBCs.size() < 2) {
    INFO("Not enough triggers in the sequence! Return -1");
    // -1 is the value set in the StarFelixTriggerLoop for invalid trigFreq
    return -1;
  }

  // 25 ns per BC
  double trigFreq = 1e9 / ( (trigBCs[1] - trigBCs[0]) * 25 ); // Hz

  if (trigBCs.size() > 2) {
    // double check using the 3rd entry
    assert( trigBCs[1] - trigBCs[0] == trigBCs[2] - trigBCs[1] );
  }

  return trigFreq;
}

TEST_CASE("StarFelixTriggerLoopNoInj", "[star][trigger_loop]") {
  std::shared_ptr<LoopActionBase> action = StdDict::getLoopAction("StarFelixTriggerLoop");

  REQUIRE ( action );

  json j;

  j["noInject"] = true;

  // Test different configurations
  SECTION("Default") {
  }

  SECTION("10 MHz 10 triggers") {
    j["trig_frequency"] = 10e6; // Hz
    j["trig_count"] = 10;
  }

  SECTION("40 MHz 103 triggers") {
    j["trig_frequency"] = 40e6; // Hz
    j["trig_count"] = 103;
  }

  SECTION("20 MHz 12321 triggers") {
    j["trig_frequency"] = 20e6; // Hz
    j["trig_count"] = 12321;
  }

  SECTION("11 MHz 71 triggers") {
    j["trig_frequency"] = 11e6; // Hz
    j["trig_count"] = 71;
  }

  SECTION("2 kHz 100001 triggers") {
    j["trig_frequency"] = 2000; // Hz
    j["trig_count"] = 100001;
  }

  SECTION("3 kHz 5 triggers") {
    j["trig_frequency"] = 3000; // Hz
    j["trig_count"] = 5;
  }

  SECTION("1 kHz 66 triggers") {
    // Very low trigger frequency: the trickle memory can only store one trigger
    // Should still send the number of triggers requested
    // The actual trigger frequency is determined by the frequency of the trickle pulse
    j["trig_frequency"] = 1000; // Hz
    j["trig_count"] = 66;
  }

  SECTION("500 Hz 66 triggers") {
    // Trigger frequency is below the threshold.
    // One trigger sequence is too long to be stored in the trickle memory
    // Should see error messages. No trigger will be sent. 
    j["trig_frequency"] = 500; // Hz
    j["trig_count"] = 66;
  }

  action->loadConfig(j);

  LoopStatusMaster ls;

  TestFwTxCore tx;
  EmptyRxCore rx;
  Bookkeeper bk(&tx, &rx);

  // Add a dummy front end with tx = 1 and rx = 0
  uint32_t lcbChn = 1;
  uint32_t rxChn = 0;
  bk.addFe(nullptr, lcbChn, rxChn);

  action->setup(&ls, &bk);
  action->execute();

  //////
  // Check the trigger words
  // The trigger word should be the trickle pulse command
  // A trickle pulse comand is 4 bytes
  uint32_t expected_trigwords = LCB_FELIX::config_command(
    LCB_FELIX::TRICKLE_TRIGGER_PULSE, 1);

  REQUIRE ( tx.m_trigWords.size() == 1 );
  REQUIRE ( tx.m_trigWords[0] == expected_trigwords );

  //////
  // Check the trigger sequence in the trickle memory
  // The trickle memory should be written via the trickle configuraiton channel
  // lcbChn + 1
  uint32_t trickleChn = lcbChn + 1;

  //
  // Number of triggers
  // Expected number of triggers
  uint32_t nTrigs_expected = std::dynamic_pointer_cast<StarFelixTriggerLoop>(action)->getTrigCnt();
  CAPTURE ( nTrigs_expected );

  // triggers per iteration of trickle memory
  uint32_t nTrigs_trickle = count_triggers(tx.m_fifo[trickleChn]);

  // Total number of triggers sent
  uint32_t nTrigs_sent = nTrigs_trickle * tx.m_nPulse;
  CAPTURE ( nTrigs_trickle, tx.m_nPulse );

  REQUIRE ( nTrigs_sent == nTrigs_expected );

  //
  // Frequency according to triggers in the tx buffer/trickle memory
  // Expected trigger frequency
  double trigFreq_expected = std::dynamic_pointer_cast<StarFelixTriggerLoop>(action)->getTrigFreq();
  CAPTURE ( trigFreq_expected );

  double trigFreq_sent = get_trig_frequency(tx.m_fifo[trickleChn]);
  CAPTURE ( trigFreq_sent );

  REQUIRE ( trigFreq_sent == trigFreq_expected );
}
