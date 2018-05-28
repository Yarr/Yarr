#include "EmuController.h"

#include "AllHwControllers.h"

#include "Fei4Emu.h"
#include "Rd53aEmu.h"

template<class FE, class ChipEmu>
std::unique_ptr<HwController> makeEmu() {
  // nikola's hack to use RingBuffer
  RingBuffer * rx = new RingBuffer(128);
  RingBuffer * tx = new RingBuffer(128);

  std::unique_ptr<HwController> ctrl(new EmuController<FE, ChipEmu>(rx, tx));

  return ctrl;
}

bool emu_registered_Fei4 =
  StdDict::registerHwController("emu",
                                makeEmu<Fei4, Fei4Emu>);

bool emu_registered_Rd53a =
  StdDict::registerHwController("emu_Rd53a",
                                makeEmu<Rd53a, Rd53aEmu>);

template<>
void EmuController<Fei4, Fei4Emu>::loadConfig(json &j) {
//    EmuTxCore::setCom(new EmuShm(j["tx"]["id"], j["tx"]["size"], true));
//    EmuRxCore::setCom(new EmuShm(j["rx"]["id"], j["rx"]["size"], true));

  auto tx = EmuTxCore<Fei4>::getCom();
  auto rx = EmuRxCore<Fei4>::getCom();

  //TODO make nice
  std::cout << "-> Starting Emulator" << std::endl;
  std::string emuCfgFile = j["feCfg"];
  std::cout << emuCfgFile << std::endl;
  emu = new Fei4Emu(emuCfgFile, emuCfgFile, rx, tx);
  emuThreads.push_back(std::thread(&Fei4Emu::executeLoop, emu));
}


template<>
void EmuController<Rd53a, Rd53aEmu>::loadConfig(json &j) {
//    EmuTxCore::setCom(new EmuShm(j["tx"]["id"], j["tx"]["size"], true));
//    EmuRxCore::setCom(new EmuShm(j["rx"]["id"], j["rx"]["size"], true));

  auto tx = EmuTxCore<Rd53a>::getCom();
  auto rx = EmuRxCore<Rd53a>::getCom();

  //TODO make nice
  std::cout << "-> Starting Emulator" << std::endl;
  std::string emuCfgFile = j["feCfg"];
  std::cout << emuCfgFile << std::endl;
  emu = new Rd53aEmu( dynamic_cast<RingBuffer*>(rx), dynamic_cast<RingBuffer*>(tx) );
  emuThreads.push_back(std::thread(&Rd53aEmu::executeLoop, emu));
}

