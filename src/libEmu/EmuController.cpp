#include "EmuController.h"

#include "AllHwControllers.h"
#include "logging.h"

#include "Fei4Emu.h"
#include "Rd53aEmu.h"

namespace {
    auto logger = logging::make_log("emu_controller");
}
/*
template<class FE, class ChipEmu>
std::unique_ptr<HwController> makeEmu() {
  // nikola's hack to use RingBuffer
  std::unique_ptr<RingBuffer> rx(new RingBuffer(128));
  std::unique_ptr<RingBuffer> tx(new RingBuffer(128));

  std::unique_ptr<HwController> ctrl(new EmuController<FE, ChipEmu>(std::move(rx), std::move(tx)));

  return ctrl;
}
*/
template<class FE, class ChipEmu>
std::unique_ptr<HwController> makeEmu() {
  auto ctrl = std::make_unique< EmuController<FE, ChipEmu> >();
  return ctrl;
}

bool emu_registered_Fei4 =
  StdDict::registerHwController("emu",
                                makeEmu<Fei4, Fei4Emu>);

bool emu_registered_Rd53a =
  StdDict::registerHwController("emu_Rd53a",
                                makeEmu<Rd53a, Rd53aEmu>);

template<>
void EmuController<Fei4, Fei4Emu>::loadConfig(const json &j) {
//    EmuTxCore::setCom(new EmuShm(j["tx"]["id"], j["tx"]["size"], true));
//    EmuRxCore::setCom(new EmuShm(j["rx"]["id"], j["rx"]["size"], true));

  // Tx EmuCom
  tx_coms.emplace_back(new RingBuffer(128));
  EmuTxCore<Fei4>::setCom(0, tx_coms.back().get());
  // Rx EmuCom
  rx_coms.emplace_back(new RingBuffer(128));
  EmuRxCore<Fei4>::setCom(0, rx_coms.back().get());

  auto tx = EmuTxCore<Fei4>::getCom(0);
  auto rx = EmuRxCore<Fei4>::getCom(0);

  //TODO make nice
  logger->info("Starting FEI4 Emulator");
  std::string emuCfgFile = j["feCfg"];
  logger->info(" read {}", emuCfgFile);
  emus.emplace_back(new Fei4Emu(emuCfgFile, emuCfgFile, rx, tx));
  emuThreads.push_back(std::thread(&Fei4Emu::executeLoop, emus.back().get()));
}


template<>
void EmuController<Rd53a, Rd53aEmu>::loadConfig(const json &j) {
//    EmuTxCore::setCom(new EmuShm(j["tx"]["id"], j["tx"]["size"], true));
//    EmuRxCore::setCom(new EmuShm(j["rx"]["id"], j["rx"]["size"], true));

  int srand_seed = time(NULL);
  std::string infotoken = "";
  if (j["seed"] == "fixed") {
    srand_seed = 1;
    infotoken = " Random Seed Fixed";
  }

  // Tx EmuCom
  tx_coms.emplace_back(new RingBuffer(128));
  EmuTxCore<Rd53a>::setCom(0, tx_coms.back().get());
  // Rx EmuCom
  rx_coms.emplace_back(new RingBuffer(128));
  EmuRxCore<Rd53a>::setCom(0, rx_coms.back().get());

  auto tx = EmuTxCore<Rd53a>::getCom(0);
  auto rx = EmuRxCore<Rd53a>::getCom(0);

  //TODO make nice
  logger->info("Starting RD53a Emulator" + infotoken);
  std::string emuCfgFile = j["feCfg"];
  logger->info(" read {}", emuCfgFile);
  emus.emplace_back(new Rd53aEmu( rx, tx, emuCfgFile, srand_seed ));
  emuThreads.push_back(std::thread(&Rd53aEmu::executeLoop, emus.back().get()));
}
