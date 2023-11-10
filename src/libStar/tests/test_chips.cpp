#include "catch.hpp"

#include <set>

#include "AllChips.h"
#include "StarChips.h"

#include "logging.h"

#include "EmptyHw.h"

namespace {

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
                            uint8_t &reg, uint32_t &value,
                            uint32_t &other) const {
    // reg = 0;
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

      if(code0 == LCB::K3) {
          // Fast command
          continue;
      }

      if(code0 == LCB::K2) {
        if(progress == 0) {
          // Start (ignore flags)
          other &= 0xffffff00;
          other |= SixEight::decode(code1);
          progress ++;
          continue;
        } else if(progress == 8) {
          // End (ignore flags)
          other &= 0x00ffffff;
          other |= SixEight::decode(code1) << 24;
          progress ++;
          continue;
        }
      }

      REQUIRE (!SixEight::is_kcode(code1));
      REQUIRE (!SixEight::is_kcode(code0));

      uint16_t data12 = (SixEight::decode(code0) << 6)
                       | SixEight::decode(code1);

      if((data12 & (~0x7f))) {
        // L0A/BCR
        continue;
      }

      if(progress == 1) {
        other &= 0xfff00fff;
        other |= (data12&0xffc) << 10;

        reg &= 0x3f;
        reg |= (data12&3)<<6;
      } else if(progress == 2) {
        reg &= 0xc0;
        reg |= (data12>>1) & 0x3f;
        // value |= (data12 & 0x7f) << (7*(7-progress)));
      } else {
        CAPTURE(data12);
        REQUIRE( progress != 0 );
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

  void loadConfig(json const&j) override {}
};

} // End namespace

TEST_CASE("StarBasicConfig", "[star][chips]") {
  MyHwController hw;

  MyTxCore &tx = hw;

  // Default is with "global" addresses
  std::string fe_name = "Star";

  SECTION("Default") {}
  SECTION("PPA") {
    fe_name = "Star_vH0A1";
  }
  SECTION("PPB") {
    fe_name = "Star_vH1A1";
  }

  auto gen_fe = StdDict::getFrontEnd(fe_name);
  auto star_fe = dynamic_cast<StarChips*> (&*gen_fe);
  REQUIRE(star_fe);

  star_fe->init(&hw, 0, 0);

  REQUIRE (tx.buffers.empty());

  star_fe->resetAllHard();

  star_fe->configure();

  REQUIRE (tx.buffers.size() > 0);

  // NB need to use one that's already registered
  auto l = spdlog::get("StarChips");

  size_t buf_count = tx.buffers.size() - 1;

#if 0
  // Just print everything that's been sent
  l->debug("  Report on {} buffers", buf_count);
  for(int i=0; i<buf_count; i++) {
    l->debug("  Buffer {} has {} words", i, tx.buffers[i].size());
    for(int f=0; f<tx.buffers[i].size() * 2; f++) {
      auto fr = tx.getFrame(i, f);
      l->debug("  frame {}: {} {:x}", i, f, fr);
    }
  }
#endif

  // This just checks that the above code can parse the commands sent
  for(int i=0; i<buf_count; i++) {
    uint8_t reg = 0xff;
    uint32_t value;
    uint32_t flags = 0xffffffff;
    tx.getRegValueForBuffer(i, reg, value, flags);
    l->debug(" reg from {:3}: {:3} {:08x} {:08x}", i, reg, value, flags);
  }
}

TEST_CASE("StarChipsNamedConfig", "[star][chips]") {
  MyHwController hw;

  MyTxCore &tx = hw;

  // Default is with "global" addresses
  auto gen_fe = StdDict::getFrontEnd("Star");
  auto star_fe = dynamic_cast<StarChips*> (&*gen_fe);
  REQUIRE(star_fe);

  star_fe->init(&hw, 0, 0);

  REQUIRE (tx.buffers.empty());

  std::set<std::pair<uint8_t, uint32_t>> expected_regs;

  SECTION("Some HCC sub regs") {
    star_fe->writeHCCRegister(48);
    star_fe->writeNamedRegister("HCC_AMENABLE", 1);
    expected_regs.insert(std::make_pair(48, 0x00406600));
    expected_regs.insert(std::make_pair(48, 0x00406601));
    // Set all 11 bits
    star_fe->writeNamedRegister("HCC_ICENABLE", 0x7ff);
    expected_regs.insert(std::make_pair(40, 0x000007ff));
    star_fe->writeNamedRegister("HCC_ICENABLE", 0x401);
    expected_regs.insert(std::make_pair(40, 0x00000401));

    // star_fe->writeNamedRegister("HCC_AMEN", 1);
  }

  SECTION("Some ABC sub regs") {
    // Send to all ABCs
    star_fe->writeABCRegister(32);
    star_fe->writeNamedRegister("ABCs_TESTPATT1", 0xa);
    star_fe->writeNamedRegister("ABCs_TESTPATT2", 0x5);

    expected_regs.insert(std::make_pair(32, 0));
    expected_regs.insert(std::make_pair(32, 0x00a00000));
    expected_regs.insert(std::make_pair(32, 0x05a00000));
  }

  REQUIRE (tx.buffers.size() > 0);

  // NB need to use one that's already registered
  auto l = spdlog::get("StarChips");

  size_t buf_count = tx.buffers.size() - 1;

#if 0
  // Just print everything that's been sent
  l->debug("  Report on {} buffers", buf_count);
  for(int i=0; i<buf_count; i++) {
    l->debug("  Buffer {} has {} words", i, tx.buffers[i].size());
    for(int f=0; f<tx.buffers[i].size() * 2; f++) {
      auto fr = tx.getFrame(i, f);
      l->debug("  frame {}: {} {:x}", i, f, fr);
    }
  }
#endif

  std::set<std::pair<uint8_t, uint32_t>> found_regs;

  // This just checks that the above code can parse the commands sent
  for(int i=0; i<buf_count; i++) {
    uint8_t reg = 0xff;
    uint32_t value;
    uint32_t flags = 0xffffffff;
    tx.getRegValueForBuffer(i, reg, value, flags);
    l->debug(" reg from {:3}: {:3} {:08x} {:08x}", i, reg, value, flags);
    found_regs.insert(std::make_pair(reg, value));
  }

  for(auto &r: expected_regs) {
    auto expected_reg = (int)r.first;
    auto expected_value = r.second;
    CAPTURE(expected_reg, expected_value);
    REQUIRE(found_regs.count(r));
  }

  for(auto &r: found_regs) {
    auto found_reg = (int)r.first;
    auto found_value = r.second;
    CAPTURE(found_reg, found_value);
    REQUIRE(expected_regs.count(r));
  }

  REQUIRE(found_regs.size() == expected_regs.size());
}

TEST_CASE("StarChips", "[star][chips]") {
  // Side-effect of checking it's not abstract is intentional
  StarChips test_config(0, 0);
  //  test_config.setHCCChipId(4);
}
