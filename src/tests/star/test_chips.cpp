#include "catch.hpp"

#include "StarChips.h"

#include "../EmptyHw.h"

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
};

class MyHwController
  : public HwController, public MyTxCore, public EmptyRxCore {
public:
  MyHwController() {}
  ~MyHwController() {}

  void loadConfig(json &j) override {}
};

TEST_CASE("StarBasicConfig", "[star][chips]") {
  MyHwController hw;

  MyTxCore &tx = hw;

  REQUIRE (tx.buffers.empty());
}

TEST_CASE("StarChips", "[star][chips]") {
  // Side-effect of checking it's not abstract is intentional
  StarChips test_config;
  //  test_config.setHCCChipId(4);
}
