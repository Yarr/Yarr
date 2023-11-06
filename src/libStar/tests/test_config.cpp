#include "catch.hpp"

#include "StarCfg.h"

TEST_CASE("StarCfg", "[star][config]") {
  int abc_version = 2;
  int hcc_version = 0;
  uint32_t creg0_response;
  std::string bad_name;
  std::string good_name;
  SECTION("With ABCv0") {
    abc_version = 0;
    creg0_response = 0x8a554321;
    bad_name = "READOUT_TIMEOUT_ENABLE";
    good_name = "A_S";
  }
  SECTION("With ABCv1") {
    abc_version = 1;
    creg0_response = 0x87669721;
    good_name = "READOUT_TIMEOUT_ENABLE";
    bad_name = "A_S";
  }

  CAPTURE (abc_version);

  // Side-effect of checking it's not abstract is intentional
  StarCfg test_config(abc_version, hcc_version);
  test_config.setHCCChipId(4);
  const int abc_id = 14;
  test_config.addABCchipID(abc_id);

  //  REQUIRE (test_config.numABCs() == 1);
  REQUIRE (test_config.getHCCchipID() == 4);

  REQUIRE (test_config.getHCCRegister(HCCStarRegister::Delay1) == 0);
  REQUIRE (test_config.getHCCRegister(HCCStarRegister::PLL1) == 0xff3b05);

  test_config.setHCCRegister(HCCStarRegister::Delay1, 0x12345678);
  REQUIRE (test_config.getHCCRegister(HCCStarRegister::Delay1) == 0x12345678);


  test_config.setABCRegister(ABCStarRegister::CREG0, 0x87654321, abc_id);
  REQUIRE (test_config.getABCRegister(ABCStarRegister::CREG0, abc_id) == 0x87654321);

  test_config.eachAbc([&](AbcCfg &abc) {

      REQUIRE (abc.getABCchipID() == abc_id);

      REQUIRE (abc.getSubRegisterParentAddr("TESTPATT1") == ABCStarRegister::CREG0);

      abc.setSubRegisterValue("TESTPATT1", 0x5);
      abc.setSubRegisterValue("TESTPATT2", 0xa);
      REQUIRE (abc.getSubRegisterValue("TESTPATT1") == 0x5);

      CHECK_THROWS (abc.getSubRegisterValue("RANDOM_NAME"));

      // Common name
      CHECK_NOTHROW (abc.getSubRegisterValue("BVT"));

      // Use abc_id here for some reason
      CHECK (test_config.getABCRegister(ABCStarRegister::CREG0, abc_id) == creg0_response);
      CHECK (abc.getSubRegisterParentValue("TESTPATT1") == creg0_response);

      // Config specific good/bad
      CHECK_THROWS (abc.getSubRegisterValue(bad_name));
      CHECK_NOTHROW (abc.getSubRegisterValue(good_name));
    });

  // Internal index for referring to the ABC
  int abc_index = 1;

  REQUIRE (test_config.getABCchipID(abc_index) == abc_id);

  REQUIRE (test_config.getSubRegisterParentAddr(abc_index, "TESTPATT1") == ABCStarRegister::CREG0);

  test_config.setSubRegisterValue(abc_index, "TESTPATT1", 0x5);
  test_config.setSubRegisterValue(abc_index, "TESTPATT2", 0xa);
  REQUIRE (test_config.getSubRegisterValue(abc_index, "TESTPATT1") == 0x5);

  REQUIRE (test_config.getABCRegister(ABCStarRegister::CREG0, abc_id) == creg0_response);
  REQUIRE (test_config.getSubRegisterParentValue(abc_index, "TESTPATT1") == creg0_response);

  json j;
  test_config.writeConfig(j);
}

// Some ABC v1 specific registers
TEST_CASE("StarCfg_ABCv1", "[star][config]") {
  int abc_version = 1;
  int hcc_version = 0;

  StarCfg test_config(abc_version, hcc_version);
  test_config.setHCCChipId(4);

  const int abc_id = 13;
  test_config.addABCchipID(abc_id);

  test_config.setABCRegister(ABCStarRegister::CREG0, 0x87654321, abc_id);
  REQUIRE (test_config.getABCRegister(ABCStarRegister::CREG0, abc_id) == 0x87654321);

  test_config.setABCRegister(ABCStarRegister::ADCS1, 0x87654321, abc_id);
  REQUIRE (test_config.getABCRegister(ABCStarRegister::ADCS1, abc_id) == 0x87654321);

  test_config.setABCRegister(ABCStarRegister::ADCS2, 0x87654321, abc_id);
  REQUIRE (test_config.getABCRegister(ABCStarRegister::ADCS2, abc_id) == 0x87654321);

  test_config.eachAbc([&](AbcCfg &abc) {
      REQUIRE (abc.getABCchipID() == abc_id);

      REQUIRE (abc.getSubRegisterParentAddr("BVREF") == ABCStarRegister::ADCS1);

      abc.setSubRegisterValue("BVREF", 0x1f);
      abc.setSubRegisterValue("BIREF", 0x1f);
      abc.setSubRegisterValue("B8BREF", 0x1f);
      abc.setSubRegisterValue("BTRANGE", 0x1f);
      abc.setSubRegisterValue("BVT", 0xff);
      abc.setSubRegisterValue("DIS_CLK", 7);
      abc.setSubRegisterValue("LCB_SELF_TEST_ENABLE", 1);

      REQUIRE (test_config.getABCRegister(ABCStarRegister::ADCS1, abc_id) == 0xffffffff);
      REQUIRE (abc.getSubRegisterParentValue("LCB_SELF_TEST_ENABLE") == 0xffffffff);

      // Others unchanged
      REQUIRE (test_config.getABCRegister(ABCStarRegister::ADCS2, abc_id) == 0x87654321);
      REQUIRE (test_config.getABCRegister(ABCStarRegister::CREG0, abc_id) == 0x87654321);
    });
}

TEST_CASE("StarCfgTrims", "[star][config]") {
  int abc_version = 0;
  int hcc_version = 0;
  StarCfg test_config(abc_version, hcc_version);
  test_config.setHCCChipId(2);
  const int abc_id = 3;
  test_config.addABCchipID(abc_id);

  // Default to 15 on all strips
  // Two rows of 128 strips (odd and even)
  for(unsigned l = 0; l < 2; l++) {
    for(unsigned s = 0; s < 128; s++) {
      CAPTURE (l, s);

      // Currently expects indices base 1
      CHECK (test_config.getTrimDAC(s+1, l+1) == 15);
    }
  }

  test_config.setTrimDAC(10, 2, 18);
  REQUIRE (test_config.getTrimDAC(10, 2) == 18);

  for(unsigned r = 0; r < 32; r++) {
    CAPTURE (r);

    // Defaults
    uint32_t expected = 0xffffffff;

    // The ones we've updated
    if (r == 2) expected = 0xffff2fff;

    CHECK (test_config.getABCRegister(ABCStarRegister::TrimLo(r), abc_id) == expected);
  }

  for(unsigned r = 0; r < 8; r++) {
    CAPTURE (r);

    // Defaults
    uint32_t expected = 0;

    // The ones we've updated
    if (r == 0) expected = 0x00080000;

    CHECK (test_config.getABCRegister(ABCStarRegister::TrimHi(r), abc_id) == expected);
  }

  // Two rows of 128 strips (odd and even)
  for(unsigned l = 0; l < 2; l++) {
    for(unsigned s = 0; s < 128; s++) {
      CAPTURE (l, s);

      int set_trim = s%32;
      // Currently expects indices base 1
      test_config.setTrimDAC(s+1, l+1, set_trim);
      CHECK (test_config.getTrimDAC(s+1, l+1) == set_trim);
    }
  }

  for(unsigned r = 0; r < 40; r++) {
    CAPTURE (r);

    uint32_t expected = 0;
    if(r<32) {
      switch(r%4) {
      case 0: expected = 0x32321010; break;
      case 1: expected = 0x76765454; break;
      case 2: expected = 0xbaba9898; break;
      case 3: expected = 0xfefedcdc; break;
      }

      CHECK (test_config.getABCRegister(ABCStarRegister::TrimLo(r), abc_id) == expected);
    } else {
      if(r%2) expected = 0xffffffff;
      CHECK (test_config.getABCRegister(ABCStarRegister::TrimHi(r-32), abc_id) == expected);
    }

  }

  //  test_config.setTrimDAC();
  // void setTrimDAC(unsigned col, unsigned row, int value);
  // int getTrimDAC(unsigned col, unsigned row);
}

TEST_CASE("Star_AbcRegInfo", "[star][config]") {
  int version;
  int write_size;

  SECTION ("With ABCv0") {
    version = 0;
    write_size = 69;
  }
  SECTION ("With ABCv1") {
    version = 1;
    write_size = 61;
  }

  CAPTURE (version);

  const auto info = AbcStarRegInfo::instance(version);

  CHECK (info->abcWriteMap.size() == write_size);

  // Check shared_ptrs refer to something
  for(auto &rm: info->abcregisterMap) {
    CAPTURE (rm.first);
    CHECK (rm.second != nullptr);
  }

  for(auto &rm: info->abcWriteMap) {
    CAPTURE (rm.first);
    CHECK (rm.second != nullptr);

    // All writeable registers should be in register list
    CHECK (info->abcregisterMap.find(rm.first) != info->abcregisterMap.end());
  }

  for(auto &rm: info->subRegMap()) {
    CAPTURE (rm.first);
    CHECK (rm.second != nullptr);
  }

  for(auto &rm: info->trimDAC_4LSB_RegisterMap_all) {
    CAPTURE (rm.first);
    CHECK (rm.second != nullptr);
  }

  for(auto &rm: info->trimDAC_1MSB_RegisterMap_all) {
    CAPTURE (rm.first);
    CHECK (rm.second != nullptr);
  }

  CHECK (info->abcWriteMap.size() == write_size);

  AbcCfg a(version);
  CHECK (a.isMasked(0) == false);
}

// Some HCC v1 specific registers
TEST_CASE("StarCfg_HCCv1", "[star][config]") {
  int abc_version = 1;
  int hcc_version = 1;

  StarCfg test_config(abc_version, hcc_version);
  test_config.setHCCChipId(4);

  const int abc_id = 13;
  test_config.addABCchipID(abc_id);

  test_config.setHCCRegister(HCCStarRegister::PLL1, 0);
  REQUIRE (test_config.getHCCRegister(HCCStarRegister::PLL1) == 0);

  HccCfg &hcc = test_config.hcc();

  REQUIRE (hcc.getSubRegisterParentAddr("EPLLPHASE160") == HCCStarRegister::PLL1);

  hcc.setSubRegisterValue("EPLLICP", 0xf);
  hcc.setSubRegisterValue("EPLLCAP", 0x3);
  hcc.setSubRegisterValue("EPLLRES", 0xf);
  hcc.setSubRegisterValue("EPLLREFFREQ", 0x3);
  hcc.setSubRegisterValue("EPLLENABLEPHASE", 0x3);
  hcc.setSubRegisterValue("EPLLPHASE160", 0x7);

  REQUIRE (hcc.getSubRegisterValue("EPLLICP") == 0xf);
  REQUIRE (hcc.getSubRegisterValue("EPLLRES") == 0xf);
  REQUIRE (hcc.getSubRegisterValue("EPLLPHASE160") == 0x7);

  uint32_t pll1 = 0xe0033f3f;

  REQUIRE (test_config.getHCCRegister(HCCStarRegister::PLL1) == pll1);
  REQUIRE (hcc.getSubRegisterParentValue("EPLLPHASE160") == pll1);

  json j;
  test_config.writeConfig(j);
}

TEST_CASE("Star_HccRegInfo", "[star][config]") {
  int version;
  int write_size;

  SECTION ("With HCCv0") {
    version = 0;
    write_size = 17;
  }
  SECTION ("With HCCv1") {
    version = 1;
    write_size = 15;
  }

  CAPTURE (version);

  const auto info = HccStarRegInfo::instance(version);

  CHECK (info->hccWriteMap.size() == write_size);

  // Check shared_ptrs refer to something
  for(auto &rm: info->hccregisterMap) {
    CAPTURE (rm.first);
    CHECK (rm.second != nullptr);
  }

  for(auto &rm: info->hccWriteMap) {
    CAPTURE (rm.first);
    CHECK (rm.second != nullptr);

    // All writeable registers should be in register list
    CHECK (info->hccregisterMap.find(rm.first) != info->hccregisterMap.end());
  }

  for(auto &rm: info->subRegMap()) {
    CAPTURE (rm.first);
    CHECK (rm.second != nullptr);
  }
}

// test that loadConfig loads basic info
TEST_CASE("StarCfgLoadConfig", "[star][config]") {
  int abc_version = 2;
  int hcc_version = 0;
  StarCfg test_config(abc_version, hcc_version);

  CHECK (test_config.getHCCchipID() == 0);
  CHECK (test_config.getHCCfuseID() == 0);

  json cfg;
  cfg["name"] = "Barrel_Hybrid_Example";
  cfg["HCC"]  = json::object();
  
  unsigned int hccID = 0x2;
  uint32_t    fuseID = 0x40086d;
  cfg["HCC"]["ID"]  = hccID;
  cfg["HCC"]["fuse_id"] = fuseID;

  //test_config.loadConfig(cfg);
  // when I add fuse_id it crashes:
  //   {Unknown expression after the reported line}
  // due to unexpected exception with message:
  //   std::get: wrong index for variant

  CHECK (test_config.getHCCchipID() == hccID);
  CHECK (test_config.getHCCfuseID() == fuseID);
}