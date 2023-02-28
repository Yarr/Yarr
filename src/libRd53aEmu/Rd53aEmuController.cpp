#include "EmuController.h"

#include "AllHwControllers.h"
#include "logging.h"

#include "Rd53aEmu.h"

namespace {
    auto logger = logging::make_log("rd5a_emu_controller");
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

bool emu_registered_Rd53a =
  StdDict::registerHwController("emu_Rd53a",
                                makeEmu<Rd53a, Rd53aEmu>);

template<>
void EmuController<Rd53a, Rd53aEmu>::loadConfig(const json &j) {
//    EmuTxCore::setCom(new EmuShm(j["tx"]["id"], j["tx"]["size"], true));
//    EmuRxCore::setCom(new EmuShm(j["rx"]["id"], j["rx"]["size"], true));

  if (j.contains("rxWaitTime")) {
    m_waitTime = std::chrono::microseconds(j["rxWaitTime"]);
  }

  int srand_seed = time(nullptr);
  std::string infotoken = "";
  if (j["seed"] == "fixed") {
    srand_seed = 1;
    infotoken = " Random Seed Fixed";
  }

  // Tx EmuCom
  tx_coms.emplace_back(new RingBuffer());
  EmuTxCore<Rd53a>::setCom(0, tx_coms.back().get());
  // Rx EmuCom
  rx_coms.emplace_back(new RingBuffer());
  EmuRxCore<Rd53a>::setCom(0, rx_coms.back().get());

  auto tx = EmuTxCore<Rd53a>::getCom(0);
  auto rx = EmuRxCore<Rd53a>::getCom(0);

  //TODO make nice
  logger->info("Starting RD53a Emulator" + infotoken);
  const json &emuCfg = j["__feCfg_data__"];
  emus.emplace_back(new Rd53aEmu( rx, tx, emuCfg, srand_seed ));
  emuThreads.push_back(std::thread(&Rd53aEmu::executeLoop, emus.back().get()));
}
