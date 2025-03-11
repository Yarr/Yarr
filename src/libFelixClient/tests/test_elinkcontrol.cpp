#include "catch.hpp"

#include "FelixController.h"

#include <string>
#include <tuple>

/*
An emulated FelixController that record the FELIX register values in a map
*/
class TestFelixController : public FelixController {

public:

  bool readFelixRegister(const std::string& registerName, uint64_t& value) override {
    if (m_registers.find(registerName) != m_registers.end()) {
      value = m_registers[registerName];
      return true;
    } else {
      return false;
    }
  }

  bool writeFelixRegister(const std::string& registerName, const std::string& regValue) override {
    m_registers[registerName] = std::stoull(regValue);
    return true;
  }

  std::map<std::string, uint64_t>& getRegMap() {return m_registers;}

  void setFwMode(FelixTools::FELIX_FW_MODE fwmode) {
    m_fwMode = fwmode;
  }

private:

  std::map<std::string, uint64_t> m_registers;

};

TEST_CASE("Link ID Conversion", "[felix]") {
  auto flxCtrl = std::make_shared<FelixController>();
  // m_did = 0 and m_cid = 0 by default

  // Link 0, elink 4
  REQUIRE (flxCtrl->FelixTxCore::fid_from_channel(4) == 0x1000000000048000 );

  // Link 4, elink 10
  REQUIRE (flxCtrl->FelixTxCore::fid_from_channel((4<<6)+10) == 0x10000000010a8000);

  // Link 0, elink 6
  REQUIRE (flxCtrl->FelixRxCore::fid_from_channel(6) == 0x1000000000060000);

  // Link 1, elink 20
  REQUIRE (flxCtrl->FelixRxCore::fid_from_channel((1<<6)+20) == 0x1000000000540000);
}

TEST_CASE("FELIX Register Names", "[felix]") {
  // IC
  REQUIRE (FelixTools::getICEnableRegName(23, true) == "MINI_EGROUP_FROMHOST_23_IC_ENABLE");
  REQUIRE (FelixTools::getICEnableRegName(1, false) == "MINI_EGROUP_TOHOST_01_IC_ENABLE"); 

  // EC
  REQUIRE (FelixTools::getECEnableRegName(2, true) == "MINI_EGROUP_FROMHOST_02_EC_ENABLE");
  REQUIRE (FelixTools::getECEnableRegName(22, false) == "MINI_EGROUP_TOHOST_22_EC_ENABLE");

  // Elink enable
  FelixTools::FelixID_t fid;
  FelixTools::FELIX_FW_MODE fwmode = FelixTools::FELIX_FW_MODE::Unknown;
  std::string regName_enable_expected, regName_width_expected;

  SECTION("ToHost") {
    fid = 0x1000000000540000; // link 1, elink 20 (egroup 5, epath 0)
    regName_enable_expected = "DECODING_LINK01_EGROUP5_CTRL_EPATH_ENA";
    regName_width_expected = "DECODING_LINK01_EGROUP5_CTRL_EPATH_WIDTH";

    SECTION("ITK_Pixel") {
      fwmode = FelixTools::FELIX_FW_MODE::ITK_Pixel;
    }

    SECTION("ITK_Strip") {
      fwmode = FelixTools::FELIX_FW_MODE::ITK_Strip;
    }
  }

  SECTION("ToFLX") {
    fid = 0x1000000000048000; // link 0, elink 4

    SECTION("ITK_Pixel") {
      fwmode = FelixTools::FELIX_FW_MODE::ITK_Pixel;
      // egroup 1 epath 0
      regName_enable_expected = "ENCODING_LINK00_EGROUP1_CTRL_EPATH_ENA";
      regName_width_expected = "ENCODING_LINK00_EGROUP1_CTRL_EPATH_WIDTH";
    }

    SECTION("ITK_Strip") {
      fwmode = FelixTools::FELIX_FW_MODE::ITK_Strip;
      // egroup 0 epath 4
      regName_enable_expected = "ENCODING_LINK00_EGROUP0_CTRL_EPATH_ENA";
      regName_width_expected = "ENCODING_LINK00_EGROUP0_CTRL_EPATH_WIDTH";
    }
  }

  auto [linkId, egroup, epath, toflx] = linkInfo_from_fid(fid, fwmode);

  REQUIRE (FelixTools::getELinkEnableRegName(fid, fwmode) == regName_enable_expected);
  REQUIRE (FelixTools::getELinkEnableRegName(linkId, egroup, toflx) == regName_enable_expected);

  REQUIRE (FelixTools::getELinkWidthRegName(fid, fwmode) == regName_width_expected);
  REQUIRE (FelixTools::getELinkWidthRegName(linkId, egroup, toflx) == regName_width_expected);

}

TEST_CASE("Elink Control", "[felix]") {
  auto flxCtrl = std::make_shared<TestFelixController>();

  SECTION("ITK_Pixel") {
    flxCtrl->setFwMode(FelixTools::FELIX_FW_MODE::ITK_Pixel);
  }

  SECTION("ITK_Strip") {
    flxCtrl->setFwMode(FelixTools::FELIX_FW_MODE::ITK_Strip);
  }

  FelixTools::FelixID_t fid1 = 0x1000000000048000;
  FelixTools::FelixID_t fid2 = 0x10000000010a8000;
  FelixTools::FelixID_t fid3 = 0x1000000000008000;

  // Enable 0x1000000000048000 and 0x10000000010a8000, and also disable all other elinks
  flxCtrl->setELinkEnableExclusive({fid1, fid2});

  // 0x1000000000048000 should be enabled
  REQUIRE (flxCtrl->getELinkEnable(fid1) == true);

  // 0x10000000010a8000 should be enabled
  REQUIRE (flxCtrl->getELinkEnable(fid2) == true);

  // Other links e.g. fid3 should be disabled
  REQUIRE (flxCtrl->getELinkEnable(fid3) == false);

  // fid1 is not exclusively enabled because fid2 is also enabled
  REQUIRE (flxCtrl->getELinkEnableExclusive({fid1}) == false);
  // Only fid1 and fid2 are enabled
  REQUIRE (flxCtrl->getELinkEnableExclusive({fid1, fid2}) == true);

  // Disable fid1 and enable fid3
  flxCtrl->setELinkEnable(fid1, false);
  flxCtrl->setELinkEnable(fid3);

  // fid1 should be disabled
  REQUIRE (flxCtrl->getELinkEnable(fid1) == false);
  
  // Only fid2 and fid3 are enabled
  REQUIRE (flxCtrl->getELinkEnableExclusive({fid2, fid3}) == true);

}