#include "catch.hpp"

#include <set>

#include "AllChips.h"
#include "StarChips.h"
#include "StarConstants.h"

#include "logging.h"

#include "EmptyHw.h"

#include "star_utils.h"

namespace {

class MyHwController
  : public HwController, public CapturePacketsTxCore, public EmptyRxCore {
public:
  MyHwController() = default;
  ~MyHwController() override = default;

  void loadConfig(json const&j) override {}
};

} // End namespace

TEST_CASE("StarBasicConfig", "[star][chips]") {
  MyHwController hw;

  CapturePacketsTxCore &tx = hw;

  // Default is with "global" addresses
  std::string fe_name = "Star";
  uint32_t set_fuse = 0;

  SECTION("Default") {}
  SECTION("PPA") {
    fe_name = "Star_vH0A1";
  }
  SECTION("PPB") {
    fe_name = "Star_vH1A1";
  }
  SECTION("PPB_with_fuse") {
    fe_name = "Star_vH1A1";
    set_fuse = 0x123456;
  }

  auto gen_fe = StdDict::getFrontEnd(fe_name);
  auto star_fe = dynamic_cast<StarChips*> (&*gen_fe);

  if(set_fuse) {
    star_fe->setHCCfuseID(set_fuse);
  }

  REQUIRE(star_fe);
  star_fe->init(&hw, FrontEndConnectivity(0,0));

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

  // Check we write the HCC communication address
  bool seenAddress = false;

  // This just checks that the above code can parse the commands sent
  for(int i=0; i<buf_count; i++) {
    RegExtractInfo rei = tx.getRegValueForBuffer(i);
    if(rei.isHccRegWrite() && (rei.reg == 17)) {
      l->info(" HCCID reg from {:3}: {:3} {:08x} {:08x}", i, rei.reg, rei.value, rei.other);
      seenAddress = true;
    }
    l->debug(" reg from {:3}: {:3} {:08x} {:08x}", i, rei.reg, rei.value, rei.other);
  }

  bool has_fuse_id = star_fe->getHCCfuseID();
  CHECK(has_fuse_id == (set_fuse!=0));

  REQUIRE (seenAddress == has_fuse_id);
}

TEST_CASE("StarChipsNamedConfig", "[star][chips]") {
  MyHwController hw;

  CapturePacketsTxCore &tx = hw;

  // Default is with "global" addresses
  auto gen_fe = StdDict::getFrontEnd("Star");
  auto star_fe = dynamic_cast<StarChips*> (&*gen_fe);
  REQUIRE(star_fe);
  star_fe->init(&hw, FrontEndConnectivity(0,0));

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
    RegExtractInfo rei = tx.getRegValueForBuffer(i);
    CHECK (rei.isRegWrite());
    l->debug(" reg from {:3}: {:3} {:08x} {:08x}", i, rei.reg, rei.value, rei.other);
    found_regs.insert(std::make_pair(rei.reg, rei.value));
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

TEST_CASE("StarChipsNamedTrim", "[star][chips][Trim]") {
  MyHwController hw;

  CapturePacketsTxCore &tx = hw;

  // Default is with "global" addresses
  auto gen_fe = StdDict::getFrontEnd("Star");
  auto star_fe = dynamic_cast<StarChips*> (&*gen_fe);
  REQUIRE(star_fe);

  // Set up a known configuration
  star_fe->clearABCchipIDs();
  star_fe->setHCCChipId(4);
  const int abc_id = 14;
  star_fe->addABCchipID(abc_id);

  star_fe->init(&hw, FrontEndConnectivity(0,0));

  REQUIRE (tx.buffers.empty());

  std::set<std::pair<uint8_t, uint32_t>> expected_regs;

  int val = 0;
  SECTION("Some trim with HI bit clear") {
    val = 6;
  }

  SECTION("Some trim with HI bit set") {
    val = 25;
  }

  // Write to TRIM registers
  // Both write registers and send to ABCs
  star_fe->writeNamedRegister("ABCs_TRIMs", val);

  star_fe->eachAbc( [&](auto &abc) {
    for(int c=0; c<Star::StripsPerABC; c++) {
      CAPTURE (c);
      CHECK (abc.getTrimDACRaw(c) == val);
    }
  });

  uint32_t lo_val = 0;
  for(int i=0; i<8; i++) {
    lo_val = (lo_val << 4) | (val & 0xf);
  }

  uint32_t hi_val = 0;
  if(val & 16) {
    hi_val = 0xffffffff;
  }

  auto read_reg0 = star_fe->getABCRegisterByID(ABCStarRegister::TrimDAC0, abc_id);
  CHECK (read_reg0 == lo_val);

  auto read_reg32 = star_fe->getABCRegisterByID(ABCStarRegister::TrimDAC32, abc_id);
  CHECK (read_reg32 == hi_val);

  for(int i=0; i<32; i++) {
    expected_regs.insert(std::make_pair((int)ABCStarRegisters::TrimLo(i), lo_val));
  }

  for(int i=0; i<8; i++) {
    expected_regs.insert(std::make_pair((int)ABCStarRegisters::TrimHi(i), hi_val));
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
    RegExtractInfo rei = tx.getRegValueForBuffer(i);
    CHECK (rei.isRegWrite());
    l->debug(" reg from {:3}: {:3} {:08x} {:08x}", i, rei.reg, rei.value, rei.other);
    found_regs.insert(std::make_pair(rei.reg, rei.value));
  }

  for(auto &r: expected_regs) {
    auto expected_reg = (int)r.first;
    auto expected_value = r.second;
    CAPTURE(expected_reg, expected_value);
    CHECK(found_regs.count(r));
  }

  for(auto &r: found_regs) {
    auto found_reg = (int)r.first;
    auto found_value = r.second;
    CAPTURE(found_reg, found_value);
    CHECK(expected_regs.count(r));
  }

  CHECK(found_regs.size() == expected_regs.size());
}

TEST_CASE("StarChips", "[star][chips]") {
  // Side-effect of checking it's not abstract is intentional
  StarChips test_config(0, 0);
  //  test_config.setHCCChipId(4);
}
