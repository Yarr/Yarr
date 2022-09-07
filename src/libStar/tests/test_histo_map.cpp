#include "catch.hpp"

#include "HccCfg.h"

TEST_CASE("CheckBuildHistoMap", "[star][histo_map]") {
  int hcc_version = 0;
  std::array<uint8_t, 11> expected_chip_map;
  int hcc_input_enables = 0;

  SECTION("Empty") {
    expected_chip_map.fill(15);
  }

  SECTION("Barrelv0") {
    hcc_version = 0;
    hcc_input_enables = 0x3ff;
    expected_chip_map = {9, 8, 7, 6, 5, 4, 3, 2, 1, 0, 15};
  }

  SECTION("Barrelv1") {
    hcc_version = 1;
    hcc_input_enables = 0x7fe;
    expected_chip_map = {15, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
  }

  SECTION("Missing both ends") {
    hcc_version = 1;
    hcc_input_enables = 0x1fc;
    expected_chip_map = {15, 15, 0, 1, 2, 3, 4, 5, 6, 15, 15};
  }

  SECTION("Missing in middle") {
    hcc_version = 1;
    hcc_input_enables = 0x70f;
    expected_chip_map = {0, 1, 2, 3, 15, 15, 15, 15, 4, 5, 6};
  }

  CAPTURE (hcc_version, hcc_input_enables);

  HccCfg hcc(hcc_version);
  hcc.setSubRegisterValue("ICENABLE", hcc_input_enables);

  auto chip_map = hcc.histoChipMap();

  REQUIRE (chip_map == expected_chip_map);
}
