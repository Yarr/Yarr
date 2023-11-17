#include "catch.hpp"

#include "AllChips.h"
#include "AllStdActions.h"
#include "LCBUtils.h"
#include "StarCfg.h"
#include "Utils.h"
#include "StarMaskLoop.h"

#include "logging.h"

#include "EmptyHw.h"

// Keep in a namespace to avoid collisions (eg test_trigger_loop)
namespace MaskTesting {

/**
   Override TxCore to record what is written to FIFO.
 */
class MyTxCore : public EmptyTxCore {
public:
  std::vector<std::vector<uint32_t>> buffers;

  void writeFifo(uint32_t word) override {
    if(buffers.empty()) {
      buffers.push_back(std::vector<uint32_t>{});
    }
    buffers.back().push_back(word);
  }

  void releaseFifo() override {
    buffers.push_back(std::vector<uint32_t>{});
  }

  /// Get LCB frame from sent words, index is in time order 
  LCB::Frame getFrame(int buff_id, size_t idx) const {
    REQUIRE (buff_id < buffers.size());

    const auto &buffer = buffers[buff_id];

    REQUIRE (idx < buffer.size() * 2);
    size_t offset = idx/2;
    // High word is first in time
    int side = (idx+1)%2;
    return (buffer[offset] >> (16*side)) & 0xffff;
  }

  void getRegValueForBuffer(int buff_id,
                            uint8_t &reg, uint32_t &value) const {
    reg = 0;
    value = 0;
    int progress = 0;
    for(int i=0; i<buffers[buff_id].size()*2; i++) {
      LCB::Frame f = getFrame(buff_id, i);
      CAPTURE (buff_id, i, f, progress);
      if(f == LCB::IDLE) continue;

      // Nothing beyond end
      REQUIRE (progress < 9);

      uint8_t code0 = (f >> 8) & 0xff;
      uint8_t code1 = f & 0xff;
      if(code0 == LCB::K2) {
        if(progress == 0) {
          // Start (ignore flags)
          progress ++;
          continue;
        } else if(progress == 8) {
          // End (ignore flags)
          progress ++;
          continue;
        }
      }

      REQUIRE (!SixEight::is_kcode(code1));
      REQUIRE (!SixEight::is_kcode(code0));

      uint16_t data12 = (SixEight::decode(code0) << 6)
                       | SixEight::decode(code1);

      if(progress == 1) {
        reg |= (data12&3)<<6;
      } else if(progress == 2) {
        reg |= (data12>>1) & 0x3f;
        // value |= (data12 & 0x7f) << (7*(7-progress)));
      } else {
        CAPTURE(data12);
        value |= (data12 & 0x7f) << (7*(7-progress));
        CAPTURE(value);
        // CHECK (progress == 2) ;
      }

      progress ++;
    }
  }
};

class MyHwController
  : public HwController, public MyTxCore, public EmptyRxCore {
public:
  MyHwController() = default;
  ~MyHwController() override = default;

  void loadConfig(const json &j) override {}

  void setupMode() override {}
  void runMode() override {}
};

} // End namespace MaskTesting

using namespace MaskTesting;

std::unique_ptr<MyTxCore> runWithConfig(json &j) {
  std::shared_ptr<LoopActionBase> action = StdDict::getLoopAction("StarMaskLoop");

  REQUIRE (action);

  action->loadConfig(j);

  if(j.contains("parameter") && j["parameter"]) {
    REQUIRE(action->isParameterLoop());
  } else {
    REQUIRE(action->isMaskLoop());
  }

  LoopStatusMaster ls;

  std::unique_ptr<MyHwController> hw(new MyHwController);
  Bookkeeper bk(&*hw, &*hw);

  MyTxCore &tx = *hw;

  auto fe = StdDict::getFrontEnd("Star").release();
  {
    auto sfe = dynamic_cast<StarCfg*> (&*fe);
    REQUIRE(sfe);
    sfe->addABCchipID(3);
  }

  fe->setActive(true);
  FrontEndConnectivity fe_conn(0,0);
  fe->init(&*hw, fe_conn);
  bk.addFe(fe, fe_conn);

  // Normally registered by LoopEngine
  ls.init(1);
  ls.addLoop(0, &*action, LOOP_STYLE_NOP);

  action->setup(&ls, &bk);
  action->execute();

  return std::move(hw);
}

void checkMaskRegisters(MyTxCore &tx, json &j,
                        uint32_t full_mask,
                        MaskType first_mask, MaskType second_mask) {
  REQUIRE (tx.buffers.size() > 0);
  // releaseFifo above creates new object
  REQUIRE (tx.buffers.back().empty());

  size_t buf_count = tx.buffers.size() - 1;
  CAPTURE (buf_count);

#if 0
  auto l = spdlog::get("StarMaskLoop");

  // Just print everything that's been sent
  l->debug("  Report on {} buffers", buf_count);
  for(int i=0; i<buf_count; i++) {
    l->debug("  Buffer {} has {} words", i, tx.buffers[i].size());
    for(int f=0; f<tx.buffers[i].size() * 2; f++) {
      auto fr = tx.getFrame(i, f);
      l->debug("  frame {}: {} {:x}", i, f, fr);
    }
  }

  for(int i=0; i<buf_count; i++) {
    uint8_t reg;
    uint32_t value;
    tx.getRegValueForBuffer(i, reg, value);
    l->debug(" reg  {}: {} {:x}", i, reg, value);
  }
#endif

  int max = j["max"];
  int step = j["step"];

  bool mask_only = !j.contains("maskOnly")
    ?false
    :(bool)j["maskOnly"];

  // 2 sets (cal and mask) of 8 registers
  unsigned regs_per_loop = mask_only ? 8 : 16;

  CAPTURE (mask_only, max, step, regs_per_loop);

  int expected_loops = max;
  expected_loops /= step;

  int loops = buf_count / regs_per_loop;

  REQUIRE (loops == expected_loops);

  std::array<uint32_t, 8> mask{0};

  for(int i=0; i<buf_count; i++) {
    uint8_t reg;
    uint32_t value;
    tx.getRegValueForBuffer(i, reg, value);

    CAPTURE(i, reg, value);

    if(reg > 0x18) {
      REQUIRE (!mask_only);
      REQUIRE ( ((reg >= 0x68) && (reg < 0x70)) );
    } else {
      REQUIRE ( ((reg >= 0x10) && (reg < 0x18)) );

      auto &test_mask = mask[reg-0x10];
      CAPTURE(test_mask, mask, ~value);

      // Not enabling anything already enabled
      REQUIRE (((~value) & test_mask) == 0);
      test_mask |= ~value;
    }

    if(i == regs_per_loop - 1) {
      REQUIRE ( mask == first_mask);
    } else if((i-regs_per_loop) == regs_per_loop - 1) {
      CAPTURE (mask);
      for(int m=0; m<8; m++) {
        CAPTURE (m);
        CAPTURE (mask[m], first_mask[m]);
        auto only_second = (mask[m]&~first_mask[m]);
        CAPTURE (only_second);
        REQUIRE (only_second == second_mask[m]);
      }
    }
  }

  for(int i=0; i<8; i++) {
    CAPTURE(i);
    REQUIRE ( mask[i] == full_mask );
  }
}

TEST_CASE("StarMaskLoop", "[star][mask_loop]") {
  // What a full mask register contains (for checking)
  uint32_t full_mask = 0xffffffff;

  MaskType first_mask;
  MaskType second_mask;

  json j;

  // SECTION means re-run this test with various options
  SECTION ("DefaultScan") {
    // Default is 0 to 127, with one channel in each bank
    j["nEnabledStripsPerGroup"] = 0;

    // Shouldn't be needed (ignored)
    j["nMaskedStripsPerGroup"] = 1;
    j["EnabledMaskedShift"] = 0;

    j["max"] = 128;
    j["min"] = 0;
    j["step"] = 1;

    first_mask = {3, 0, 0, 0, 0, 0, 0, 0};
    second_mask = {0xc, 0, 0, 0, 0, 0, 0, 0};
  }
  SECTION ("AnalogueScan") {
    // This enables one in 8 channels
    j["nMaskedStripsPerGroup"] = 1;
    j["nEnabledStripsPerGroup"] = 1;
    j["EnabledMaskedShift"] = 0;
    j["max"] = 8;
    j["min"] = 0;
    j["step"] = 1;

    first_mask = {0x00030003, 0x00030003, 0x00030003, 0x00030003,
                  0x00030003, 0x00030003, 0x00030003, 0x00030003};
    second_mask = {0x000c000c, 0x000c000c, 0x000c000c, 0x000c000c,
                   0x000c000c, 0x000c000c, 0x000c000c, 0x000c000c};
  }
  SECTION ("AnalogueScan_32") {
    j["nMaskedStripsPerGroup"] = 1;
    j["nEnabledStripsPerGroup"] = 1;
    j["EnabledMaskedShift"] = 0;
    j["max"] = 32;
    j["min"] = 0;
    j["step"] = 1;

    first_mask = {3, 0, 3, 0, 3, 0, 3, 0};
    second_mask = {0xc, 0, 0xc, 0, 0xc, 0, 0xc, 0};
  }
  SECTION ("ScanParameter") {
    j["nMaskedStripsPerGroup"] = 1;
    j["nEnabledStripsPerGroup"] = 1;
    j["EnabledMaskedShift"] = 0;
    j["max"] = 32;
    j["min"] = 0;
    j["step"] = 1;
    j["parameter"] = 1;
    first_mask = {3, 0, 3, 0, 3, 0, 3, 0};
    second_mask = {0xc, 0, 0xc, 0, 0xc, 0, 0xc, 0};
  }
  SECTION ("AnalogueScan_NoCal") {
    // Don't change calibration mask register
    j["nMaskedStripsPerGroup"] = 1;
    j["nEnabledStripsPerGroup"] = 1;
    j["EnabledMaskedShift"] = 0;
    j["max"] = 8;
    j["min"] = 0;
    j["step"] = 1;
    j["maskOnly"] = true;
    first_mask = {0x00030003, 0x00030003, 0x00030003, 0x00030003,
                  0x00030003, 0x00030003, 0x00030003, 0x00030003};
    second_mask = {0x000c000c, 0x000c000c, 0x000c000c, 0x000c000c,
                   0x000c000c, 0x000c000c, 0x000c000c, 0x000c000c};
  }
  SECTION ("AnalogueScan_32_step2") {
    // Mask one in 32, but step by 2 each iteration, misses half the channels
    j["nMaskedStripsPerGroup"] = 1;
    j["nEnabledStripsPerGroup"] = 1;
    j["EnabledMaskedShift"] = 0;
    j["max"] = 32;
    j["min"] = 0;
    j["step"] = 2;

    // Step of 2
    full_mask = 0x33333333;
    first_mask = {3, 0, 3, 0, 3, 0, 3, 0};
    second_mask = {0x30, 0, 0x30, 0, 0x30, 0, 0x30, 0};
  }

  std::unique_ptr<MyTxCore> tx_ptr(std::move(runWithConfig(j)));
  auto tx = *tx_ptr;

  checkMaskRegisters(tx, j, full_mask, first_mask, second_mask);
}

TEST_CASE("StarMaskLoopNmask", "[star][mask_loop]") {
  json j;

  // SECTION means re-run this test with various options
  SECTION ("NmaskScan") {
    j["doNmask"] = true;
    j["maskOnly"] = true;

    // Shouldn't be needed (ignored)
    j["nEnabledStripsPerGroup"] = 0;
    j["nMaskedStripsPerGroup"] = 0;
    j["EnabledMaskedShift"] = 0;

    j["max"] = 128;
    j["min"] = 0;
    j["step"] = 1;
  }

  std::unique_ptr<MyTxCore> tx_ptr(std::move(runWithConfig(j)));
  auto tx = *tx_ptr;

  REQUIRE (tx.buffers.size() > 0);
  // releaseFifo above creates new object
  REQUIRE (tx.buffers.back().empty());

  size_t buf_count = tx.buffers.size() - 1;
  CAPTURE (buf_count);

  int max = j["max"];
  int step = j["step"];

  bool mask_only = !j.contains({"maskOnly"})
    ?false
    :(bool)j["maskOnly"];

  // 2 sets (cal and mask) of 8 registers
  unsigned regs_per_loop = mask_only ? 8 : 16;

  CAPTURE (mask_only, max, step, regs_per_loop);

  int expected_loops = max;
  expected_loops /= step;

  int loops = buf_count / regs_per_loop;

  REQUIRE (loops == expected_loops);

  std::array<uint32_t, 8> mask{0};

  for(int i=0; i<buf_count; i++) {
    uint8_t reg;
    uint32_t value;
    tx.getRegValueForBuffer(i, reg, value);

    CAPTURE(i, reg, value);

    if(reg > 0x18) {
      REQUIRE (!mask_only);
      REQUIRE ( ((reg >= 0x68) && (reg < 0x70)) );
    } else {
      REQUIRE ( ((reg >= 0x10) && (reg < 0x18)) );

      // std::cout << "Mask: " << (reg-0x10) << " " << std::bitset<32>(value) << "\n";

      auto &test_mask = mask[reg-0x10];
      CAPTURE(test_mask, mask, ~value);
      CAPTURE((~value) & test_mask);

      // Not enabling anything already enabled
      REQUIRE (((~value) & test_mask) == 0);
      test_mask = value;
    }

    if(((i+1)%regs_per_loop) == 0) {
      int iter = i/regs_per_loop;
      CAPTURE(i, iter);

      std::array<uint32_t, 8> exp_mask;
      exp_mask.fill(0xffffffff);
      if(iter == 0) {
        REQUIRE ( mask == exp_mask);
      } else if(iter == 4) {
        exp_mask[0] = 0xffffffaa;
        REQUIRE ( mask == exp_mask);
      } else if(iter == 16) {
        exp_mask[0] = 0xaaaaaaaa;
        REQUIRE ( mask == exp_mask);
      } else if(iter == 64) {
        exp_mask[0] = 0xaaaaaaaa;
        exp_mask[1] = 0xaaaaaaaa;
        exp_mask[2] = 0xaaaaaaaa;
        exp_mask[3] = 0xaaaaaaaa;
        REQUIRE ( mask == exp_mask);
      } else if(iter == 120) {
        exp_mask[0] = 0xaaaaaaaa;
        exp_mask[1] = 0xaaaaaaaa;
        exp_mask[2] = 0xaaaaaaaa;
        exp_mask[3] = 0xaaaaaaaa;
        exp_mask[4] = 0xaaaaaaaa;
        exp_mask[5] = 0xaaaaaaaa;
        exp_mask[6] = 0xaaaaaaaa;
        exp_mask[7] = 0xffffaaaa;
        REQUIRE ( mask == exp_mask);
      }

      mask = std::array<uint32_t, 8>{};
    }
  }
}

TEST_CASE("StarMaskLoopRing", "[star][mask_loop]") {
  ChannelRing ring;
  for(int i=0; i<128; i++) {
    ring.fill(false);
    ring.fill(true);
  }

  REQUIRE ( ring.readMask(0) == MaskType{0x33333333, 0x33333333, 0x33333333, 0x33333333, 0x33333333, 0x33333333, 0x33333333, 0x33333333} );
  REQUIRE ( ring.readMask(1) == MaskType{0xcccccccc, 0xcccccccc, 0xcccccccc, 0xcccccccc, 0xcccccccc, 0xcccccccc, 0xcccccccc, 0xcccccccc} );

  REQUIRE ( ring.readCalEnable(0) == MaskType{0xaaaaaaaa, 0xaaaaaaaa, 0xaaaaaaaa, 0xaaaaaaaa, 0xaaaaaaaa, 0xaaaaaaaa, 0xaaaaaaaa, 0xaaaaaaaa} );
  REQUIRE ( ring.readCalEnable(1) == MaskType{0x55555555, 0x55555555, 0x55555555, 0x55555555, 0x55555555, 0x55555555, 0x55555555, 0x55555555} );
  REQUIRE ( ring.readCalEnable(2) == MaskType{0xaaaaaaaa, 0xaaaaaaaa, 0xaaaaaaaa, 0xaaaaaaaa, 0xaaaaaaaa, 0xaaaaaaaa, 0xaaaaaaaa, 0xaaaaaaaa} );
  REQUIRE ( ring.readCalEnable(4) == MaskType{0xaaaaaaaa, 0xaaaaaaaa, 0xaaaaaaaa, 0xaaaaaaaa, 0xaaaaaaaa, 0xaaaaaaaa, 0xaaaaaaaa, 0xaaaaaaaa} );

  // Mostly checking which end we fill from
  ChannelRing ring_one;
  ring_one.fill(true);
  for(int i=0; i<256; i++) {
    ring.fill(false);
  }

  REQUIRE ( ring_one.readMask(0) == MaskType{0xfffffffe, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff} );
  REQUIRE ( ring_one.readCalEnable(0) == MaskType{1, 0, 0, 0, 0, 0, 0, 0} );
}
