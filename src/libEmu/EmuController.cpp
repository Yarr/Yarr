#include "EmuController.h"

#include "AllHwControllers.h"

#include "Fei4Emu.h"

std::unique_ptr<HwController> makeEmu() {
  // nikola's hack to use RingBuffer
  RingBuffer * rx = new RingBuffer(128);
  RingBuffer * tx = new RingBuffer(128);

  std::unique_ptr<HwController> ctrl(new EmuController(rx, tx));

  return ctrl;
}

bool emu_registered =
  StdDict::registerHwController("emu",
                                makeEmu);

EmuController::EmuController(RingBuffer * rx, RingBuffer * tx)
  : emu(nullptr) {
    EmuTxCore::setCom(tx);
    EmuRxCore::setCom(rx);
}

EmuController::~EmuController() {
  if(emu) {
    emu->run = false;
  }
  emuThreads[0].join();
}

void EmuController::loadConfig(json &j) {
//    EmuTxCore::setCom(new EmuShm(j["tx"]["id"], j["tx"]["size"], true));
//    EmuRxCore::setCom(new EmuShm(j["rx"]["id"], j["rx"]["size"], true));

  auto tx = EmuTxCore::getCom();
  auto rx = EmuRxCore::getCom();

  //TODO make nice
  std::cout << "-> Starting Emulator" << std::endl;
  std::string emuCfgFile = j["feCfg"];
  std::cout << emuCfgFile << std::endl;
  emu = new Fei4Emu(emuCfgFile, emuCfgFile, rx, tx);
  emuThreads.push_back(std::thread(&Fei4Emu::executeLoop, emu));
}
