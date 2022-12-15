#include "catch.hpp"

#include "AllHwControllers.h"
#include "AllChips.h"
#include "Bookkeeper.h"
#include "FrontEnd.h"
#include "StarChips.h"
#include "StarChipsBroadcast.h"
#include "StarChipPacket.h"
#include "EmuController.h"
#include "StarEmu.h"

// Read an ABC RR packet
std::pair<unsigned, unsigned> readABCRRPacket(HwController* ctrl, unsigned maxTries=10) {
  StarChipPacket packet;

  for (unsigned i=0; i<maxTries; i++) {
    CAPTURE(i);
    auto dataVec = ctrl->readData();

    CAPTURE(dataVec.size());
    for (const auto& data : dataVec) {
      if (not data) continue;

      // Parse the raw data
      packet.clear();
      packet.add_word(0x13c); // add SOP
      for (unsigned iw=0; iw<data->getSize(); iw++) {
        for (int ib=0; ib<4; ib++) {
          packet.add_word((data->get(iw)>>ib*8)&0xff);
        }
      }
      packet.add_word(0x1dc); // add EOP

      bool parse_failed = packet.parse();
      CAPTURE(parse_failed);

      if (parse_failed) {
        CAPTURE(packet.raw_word_string());
        continue;
      }

      if (packet.getType() == TYP_ABC_RR) {
        return std::make_pair(packet.address, packet.value);
      }
    }
  }

  // Shouldn't reach here
  REQUIRE(packet.getType() == TYP_ABC_RR);
  return std::make_pair(packet.address, packet.value);
}

// Use Star emulator to test StarChipsBroadcast
TEST_CASE("StarBroadcast", "[star][chips][emuulator]") {

  //////
  // Prepare configurations

  // HCC with ID = 1
  // Two ABCs: ID = 9 and 8
  json chipCfg1;
  chipCfg1["HCC"] = {{"ID", 1}};
  chipCfg1["ABCs"] = {{"IDs", {9, 8}}};

  // Set sub-register "STR_DEL" to different values for ABCs
  chipCfg1["ABCs"]["subregs"] = json::array();
  chipCfg1["ABCs"]["subregs"][0] = {{"STR_DEL", 12}};
  chipCfg1["ABCs"]["subregs"][1] = {{"STR_DEL", 21}};

  // Write to file
  std::ofstream fileChipCfg1("tmp_star_1.json");
  fileChipCfg1 << chipCfg1;
  fileChipCfg1.close();

  // Another emulated front end
  // HCC with ID = 2
  // Two ABCs: ID = 1 and 2
  json chipCfg2;
  chipCfg2["HCC"] = {{"ID", 2}};
  chipCfg2["ABCs"] = {{"IDs", {1, 2}}};

  // Set sub-register "STR_DEL" to different values for ABCs
  chipCfg2["ABCs"]["subregs"] = json::array();
  chipCfg2["ABCs"]["subregs"][0] = {{"STR_DEL", 13}};
  chipCfg2["ABCs"]["subregs"][1] = {{"STR_DEL", 30}};

  // Write to file
  std::ofstream fileChipCfg2("tmp_star_2.json");
  fileChipCfg2 << chipCfg2;
  fileChipCfg2.close();

  // Connectivity config
  json connectivity;
  connectivity["chips"] = json::array();
  // The two FEs are on the same command segment
  connectivity["chips"][0] = {{"tx", 1}, {"rx", 0}, {"config","tmp_star_1.json"}};
  connectivity["chips"][1] = {{"tx", 1}, {"rx", 2}, {"config","tmp_star_2.json"}};
  // Write to file
  std::ofstream fileConnectCfg("tmp_star_connectivity.json");
  fileConnectCfg << connectivity;
  fileConnectCfg.close();

  //////
  // Configure the emulator controller
  std::unique_ptr<HwController> emu = StdDict::getHwController("emu_Star");

  REQUIRE(emu);

  json emuCfg;
  emuCfg["chipCfg"] = "tmp_star_connectivity.json";
  emu->loadConfig(emuCfg);

  // The config files can be deleted now
  remove("tmp_star_connectivity.json");
  remove("tmp_star_1.json");
  remove("tmp_star_2.json");

  //////
  // Configure the front ends
  Bookkeeper bk(emu.get(), emu.get());

  // Add the first FE
  unsigned tx0 = connectivity["chips"][0]["tx"];
  unsigned rx0 = connectivity["chips"][0]["rx"];
  bk.addFe(StdDict::getFrontEnd("Star").release(), tx0, rx0);
  bk.getLastFe()->init(emu.get(), tx0, rx0);
  auto star1 = dynamic_cast<StarChips*>(bk.getLastFe());
  REQUIRE(star1);
  star1->loadConfig(chipCfg1);

  // Add the second FE
  unsigned tx1 = connectivity["chips"][1]["tx"];
  unsigned rx1 = connectivity["chips"][1]["rx"];
  bk.addFe(StdDict::getFrontEnd("Star").release(), tx1, rx1);
  bk.getLastFe()->init(emu.get(), tx1, rx1);
  auto star2 = dynamic_cast<StarChips*>(bk.getLastFe());
  REQUIRE(star2);
  star2->loadConfig(chipCfg2);

  //////
  // Initialize global FE
  bk.initGlobalFe("Star");

  /* The following initialization of global Fe would fail the test. */
  //bk.initGlobalFe(StdDict::getFrontEnd("Star").release());
  //bk.getGlobalFe()->makeGlobal();

  bk.getGlobalFe()->init(emu.get(), 0, 0);

  // Use the global FE to update a sub-register "BCAL" of all FEs
  // BCAL is in the same register as STR_DEL
  REQUIRE(star1->getSubRegisterParentAddr(1,"BCAL") == star1->getSubRegisterParentAddr(1,"STR_DEL"));

  bk.getGlobalFe()->writeNamedRegister("ABCs_BCAL", 66);
  while(not emu->isCmdEmpty());

  ///////
  // Check the sub-register "STR_DEL" in all emulated ABC chips
  // STR_DEL in each ABC should not be overwritten

  // A dummy StarCfg object to help extract the sub-register value
  StarCfg dummyCfg(0, 0);
  dummyCfg.setHCCChipId(0xf);
  dummyCfg.addABCchipID(0xf);

  // Send register read command
  emu->setRxEnable(rx0);
  star1->readABCSubRegister("STR_DEL", 9);
  while(not emu->isCmdEmpty());
  // Read RR packet
  auto [addr1, val1] = readABCRRPacket(emu.get());
  CAPTURE(addr1);
  CAPTURE(val1);

  dummyCfg.setABCRegister(addr1, val1, 0xf);
  // Expected STR_DEL: 12
  REQUIRE(dummyCfg.getSubRegisterValue(1, "STR_DEL") == 12);

  // Send register read command
  star1->readABCSubRegister("STR_DEL", 8);
  while(not emu->isCmdEmpty());
  // Read RR packet
  auto [addr2, val2] = readABCRRPacket(emu.get());
  CAPTURE(addr2);
  CAPTURE(val2);

  dummyCfg.setABCRegister(addr2, val2, 0xf);
  // Expected STR_DEL: 21
  REQUIRE(dummyCfg.getSubRegisterValue(1, "STR_DEL") == 21);

  emu->setRxEnable(rx1);
  star2->readABCSubRegister("STR_DEL", 1);
  while(not emu->isCmdEmpty());
  // Read RR packet
  auto [addr3, val3] = readABCRRPacket(emu.get());
  CAPTURE(addr3);
  CAPTURE(val3);

  dummyCfg.setABCRegister(addr3, val3, 0xf);
  // Expected STR_DEL: 13
  REQUIRE(dummyCfg.getSubRegisterValue(1, "STR_DEL") == 13);

  star2->readABCSubRegister("STR_DEL", 2);
  while(not emu->isCmdEmpty());
  // Read RR packets
  auto [addr4, val4] = readABCRRPacket(emu.get());
  CAPTURE(addr4);
  CAPTURE(val4);

  dummyCfg.setABCRegister(addr4, val4, 0xf);
  // Expected STR_DEL: 30
  REQUIRE(dummyCfg.getSubRegisterValue(1, "STR_DEL") == 30);
}