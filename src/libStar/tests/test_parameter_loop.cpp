#include "catch.hpp"

#include "AllChips.h"
#include "AllStdActions.h"
#include "Bookkeeper.h"
#include "LCBUtils.h"
#include "StarCfg.h"
#include "Utils.h"
#include "StarMaskLoop.h"

#include "logging.h"

#include "EmptyHw.h"
#include "star_utils.h"

// Keep in a namespace to avoid collisions
namespace StripParamLoopTesting {

class MyHwController
  : public HwController, public CapturePacketsTxCore, public EmptyRxCore {
public:
  MyHwController() = default;
  ~MyHwController() override = default;

  void loadConfig(const json &j) override {}

  void setupMode() override {}
  void runMode() override {}
};

// Run loop of StdParameterLoop with the provided configuration
std::unique_ptr<CapturePacketsTxCore> runWithConfig(json &j) {
  std::shared_ptr<LoopActionBase> action = StdDict::getLoopAction("StdParameterLoop");

  REQUIRE (action);

  action->loadConfig(j);

  REQUIRE(action->isParameterLoop());

  LoopStatusMaster ls;

  std::unique_ptr<MyHwController> hw(new MyHwController);
  Bookkeeper bk(&*hw, &*hw);

  CapturePacketsTxCore &tx = *hw;

  auto fe = StdDict::getFrontEnd("Star");
  {
    auto sfe = dynamic_cast<StarCfg*> (fe.get());
    sfe->clearABCchipIDs();
    REQUIRE(sfe);
    sfe->addABCchipID(3);
    REQUIRE(sfe->numABCs() == 1);
  }
  bk.initGlobalFe("Star");
  
  fe->setActive(true);
  FrontEndConnectivity fe_conn(0,0);
  fe->init(&*hw, fe_conn);
  bk.addFe(std::move(fe), fe_conn);

  // Normally registered by LoopEngine
  ls.init(1);
  ls.addLoop(0, &*action, LOOP_STYLE_NOP);

  action->setup(&ls, &bk);
  action->execute();

  return std::move(hw);
}

struct RegInfo {
  uint8_t addr;
  uint32_t value;
};

void checkRegisters(CapturePacketsTxCore &tx, json &j,
                    size_t total_count,
                    const std::vector<RegInfo> &regs
                    ) {
  REQUIRE (tx.buffers.size() > 0);
  // releaseFifo above creates new object
  REQUIRE (tx.buffers.back().empty());

  size_t buf_count = tx.buffers.size() - 1;
  CAPTURE (buf_count);

#if 1
  auto l = spdlog::get("StdParameterLoop");

  // Just print all registers that have been sent
  for(int i=0; i<buf_count; i++) {
    auto rei = tx.getRegValueForBuffer(i);
    l->debug("TEST: Write reg idx {:3}: addr {} val {:x}", i, rei.reg, rei.value);
  }
#endif

  if(total_count != -1)  {
    REQUIRE(total_count == buf_count);
  }

  for(auto &r: regs) {
    CAPTURE (r.addr, r.value);

    bool found = false;

    for(int i=0; i<buf_count; i++) {
      auto rei = tx.getRegValueForBuffer(i);
      if(rei.reg != r.addr || rei.value != r.value) {
        continue;
      }
      found = true;
      break;
    }

    REQUIRE( found );
  }
}

} // End namespace StripParamLoopTesting

using namespace StripParamLoopTesting;

TEST_CASE("StarParameterLoop", "[star][parameter_loop]") {
  // // What a full mask register contains (for checking)
  // uint32_t full_mask = 0xffffffff;

  // MaskType first_mask;
  // MaskType second_mask;

  json j;

  std::vector<RegInfo> reg_list;

  // SECTION means re-run this test with various options
  SECTION ("LSBTrimScan") {
    j["parameter"] = "ABCs_TRIMs";

    j["max"] = 2;
    j["min"] = 0;
    j["step"] = 1;

    // LSB sets 8 nibbles
    reg_list = {
      {64, 0x00000000},
      {64, 0x11111111},
      {64, 0x22222222},
      {95, 0x00000000},
      {95, 0x11111111},
      {95, 0x22222222},
    };
  }

  SECTION ("MSBTrimScan") {
    j["parameter"] = "ABCs_TRIMs";

    j["max"] = 16;
    j["min"] = 0;
    j["step"] = 16;

    // All 32 bits set to 1
    reg_list = {
      {96, 0x00000000},
      {96, 0xffffffff},
      {103, 0x00000000},
      {103, 0xffffffff},
    };
  }

  SECTION ("SimpleMaskScan") {
    j["parameter"] = "ABCs_MASKs";

    j["min"] = 0;
    j["max"] = 1;
    j["step"] = 1;

    reg_list = {
      {16, 0x00000000},
      {23, 0x00000000},
      {16, 0xffffffff},
      {23, 0xffffffff},
    };
  }

  std::unique_ptr<CapturePacketsTxCore> tx_ptr(std::move(runWithConfig(j)));
  auto tx = *tx_ptr;

  checkRegisters(tx, j, -1, reg_list);
}
