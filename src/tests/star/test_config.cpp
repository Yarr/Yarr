#include "catch.hpp"

#include "StarCfg.h"

TEST_CASE("StarCfg", "[star][config]") {
  // Side-effect of checking it's not abstract is intentional
  StarCfg test_config;
  test_config.setHCCChipId(4);
  const int abc_id = 14;
  test_config.addABCchipID(abc_id);
  test_config.m_nABC ++;

  // This sets up initial values
  test_config.initRegisterMaps();

  //  REQUIRE (test_config.numABCs() == 1);
  REQUIRE (test_config.getHCCchipID() == 4);
  REQUIRE (test_config.getABCchipID(1) == abc_id);

  REQUIRE (test_config.getHCCRegister(HCCStarRegister::Delay1) == 0);
  REQUIRE (test_config.getHCCRegister(HCCStarRegister::PLL1) == 0xff3b05);

  test_config.setHCCRegister(HCCStarRegister::Delay1, 0x12345678);
  REQUIRE (test_config.getHCCRegister(HCCStarRegister::Delay1) == 0x12345678);


  test_config.setABCRegister(ABCStarRegister::CREG0, 0x87654321, abc_id);
  REQUIRE (test_config.getABCRegister(ABCStarRegister::CREG0, abc_id) == 0x87654321);

  REQUIRE (test_config.getSubRegisterParentAddr(1, "TESTPATT1") == ABCStarRegister::CREG0);

  test_config.setSubRegisterValue(1, "TESTPATT1", 0x5);
  test_config.setSubRegisterValue(1, "TESTPATT2", 0xa);
  REQUIRE (test_config.getSubRegisterValue(1, "TESTPATT1") == 0x5);

  REQUIRE (test_config.getABCRegister(ABCStarRegister::CREG0, abc_id) == 0x8a554321);
  REQUIRE (test_config.getSubRegisterParentValue(1, "TESTPATT1") == 0x8a554321);
}
