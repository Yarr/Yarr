#include "catch.hpp"

#include "AllStdActions.h"
#include "LCBUtils.h"
#include "Utils.h"

#include "../EmptyHw.h"

/**
   Override TxCore to record what trigger word is set to.
 */
class MyTxCore : public EmptyTxCore {
public:
  std::vector<uint32_t> last_buffer;
  uint32_t last_length;

  void setTrigWordLength(uint32_t length) override {
    last_length = length;
  }

  void setTrigWord(uint32_t *word, uint32_t length) override {
    last_buffer.resize(length);
    for(int i=0; i<length; i++) {
      last_buffer[i] = word[i];
    }    
  }

  /// Get LCB frame from sent trigger word, index is in time order 
  LCB::Frame getFrame(size_t idx) {
    REQUIRE (idx < (last_length * 2) );
    size_t offset = (last_length-1)-(idx/2);
    int side = (idx+1)%2;
    return (last_buffer[offset] >> (16*side)) & 0xffff;
  }
};

TEST_CASE("StarTriggerLoopDelay", "[star][trigger_loop]") {
  std::shared_ptr<LoopActionBase> action = StdDict::getLoopAction("StarTriggerLoop");

  REQUIRE (action);

  json j;

  // These are some parameters that change what is expected from running
  // the different SECTIONs

  // Index of frame generating the calibration pulse
  int pulse_index = 0;
  // Index of frame generating the trigger
  int trigger_index = 12;
  int expected_bc_slot = 2;

  int expected_l0_mask = 1; // ie 3rd BC of that block

  // This means we run this test with various options
  SECTION("Default") {
  }
  SECTION("Zero latency") {
    j["l0_latency"] = 0;

    pulse_index = 1;
    trigger_index = 2;
    expected_bc_slot = 3;
  }
  SECTION("Latency = 1") {
    j["l0_latency"] = 1;

    pulse_index = 1;
    trigger_index = 2;
    expected_bc_slot = 2;
  }
  SECTION("Latency = 4") {
    j["l0_latency"] = 4;

    pulse_index = 0;
    trigger_index = 2;
    expected_bc_slot = 3;
  }
  SECTION("Latency = 7") {
    j["l0_latency"] = 7;

    pulse_index = 0;
    trigger_index = 2;
    expected_bc_slot = 0;
  }
  SECTION("Latency = 10") {
    j["l0_latency"] = 10;

    pulse_index = 1;
    trigger_index = 4;
    expected_bc_slot = 1;
  }
  SECTION("No charge injection") {
    j["noInject"] = true;

    pulse_index = -1;
    trigger_index = 0;
    expected_bc_slot = 3;

    expected_l0_mask = 4;
  }

  // Calculate delay based on used slots
  int delay = 3-expected_bc_slot;
  delay += (trigger_index - pulse_index) * 4;
  // Don't count the 4 BCs to send the trigger
  delay -= 4;
  INFO ( "Calculate delay: " << delay );

  if(!j["l0_latency"].empty()) {
    int l0_latency = j["l0_latency"];
    REQUIRE ( delay == l0_latency );
  }

  action->loadConfig(j);

  LoopStatusMaster ls;

  MyTxCore tx;
  EmptyRxCore rx;
  Bookkeeper bk(&tx, &rx);

  action->setup(&ls, &bk);
  action->execute();

  REQUIRE (tx.last_buffer.size() > 0);

#if 0
  for(int i=0; i<tx.last_length*2; i++) {
    auto f = tx.getFrame(i);
    std::cout << "  " << i << ": " << Utils::hexify(f) << "\n";
  }
#endif

  for(int i=0; i<tx.last_length*2; i++) {
    auto f = tx.getFrame(i);

    if(i == pulse_index) continue;
    if(i == trigger_index) continue;

    CAPTURE (pulse_index, trigger_index, i );

    REQUIRE ( f == LCB::IDLE );
  }

  if(pulse_index != -1) {
    auto pulse_frame = tx.getFrame(pulse_index);

    CAPTURE ( pulse_frame );

    uint8_t f, s;
    std::tie(f, s) = LCB::split_pair(pulse_frame);
    CAPTURE ( pulse_frame );
    CAPTURE ( pulse_frame >> 8, pulse_frame & 0xff);

    REQUIRE ( LCB::is_valid(pulse_frame) );

    // REQUIRE ( LCB::is_fast_command(LCB::fast_command(LCB::ABC_CAL_PULSE, 0)));
    REQUIRE ( pulse_frame != LCB::IDLE );

    REQUIRE ( !LCB::is_l0a_bcr(pulse_frame) );

    REQUIRE ( LCB::is_fast_command(pulse_frame) );

    int bc_slot = LCB::get_fast_command_bc(pulse_frame);
    REQUIRE ( pulse_frame == LCB::fast_command(LCB::ABC_CAL_PULSE, bc_slot) );
    REQUIRE ( bc_slot == expected_bc_slot );
  }

  auto trigger_frame = tx.getFrame(trigger_index);
  CAPTURE ( trigger_frame );
  REQUIRE ( LCB::is_valid(trigger_frame) );
  REQUIRE ( LCB::is_l0a_bcr(trigger_frame) );

  CAPTURE( LCB::is_bcr(trigger_frame),
           LCB::get_l0_mask(trigger_frame),
           LCB::get_l0_tag(trigger_frame) );
  REQUIRE ( !LCB::is_bcr(trigger_frame) );

  REQUIRE ( LCB::get_l0_mask(trigger_frame) == expected_l0_mask );
}
