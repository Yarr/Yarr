#include "catch.hpp"

#include "AllChips.h"
#include "AllStdActions.h"
#include "LCBUtils.h"
#include "StarCfg.h"
#include "Utils.h"

#include "logging.h"

#include "../EmptyHw.h"

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
  MyHwController() {}
  virtual ~MyHwController() {}

  void loadConfig(json &j) override {}

  void setupMode() override {}
  void runMode() override {}
};

} // End namespace MaskTesting

using namespace MaskTesting;

std::unique_ptr<MyTxCore> runWithConfig(json &j) {
  std::shared_ptr<LoopActionBase> action = StdDict::getLoopAction("StarMaskLoop");

  REQUIRE (action);

  action->loadConfig(j);

  REQUIRE(action->isMaskLoop());

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
  fe->init(&*hw, 0, 0);
  bk.addFe(fe, 0, 0);

  // Normally registered by LoopEngine
  ls.init(1);
  ls.addLoop(0, &*action);

  action->setup(&ls, &bk);
  action->execute();

  return std::move(hw);
}

void checkMaskRegisters(MyTxCore &tx, json &j, uint32_t full_mask) {
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

  bool mask_only = j["maskOnly"].empty()
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
  }

  for(int i=0; i<8; i++) {
    CAPTURE(i);
    REQUIRE ( mask[i] == full_mask );
  }
}

TEST_CASE("StarMaskLoop", "[star][mask_loop]") {
  // What a full mask register contains (for checking)
  uint32_t full_mask = 0xffffffff;

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
  }
  SECTION ("AnalogueScan") {
    // This enables one in 8 channels
    j["nMaskedStripsPerGroup"] = 1;
    j["nEnabledStripsPerGroup"] = 1;
    j["EnabledMaskedShift"] = 0;
    j["max"] = 8;
    j["min"] = 0;
    j["step"] = 1;
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
  }

  std::unique_ptr<MyTxCore> tx_ptr(std::move(runWithConfig(j)));
  auto tx = *tx_ptr;

  checkMaskRegisters(tx, j, full_mask);
}
