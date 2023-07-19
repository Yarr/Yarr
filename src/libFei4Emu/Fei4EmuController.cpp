#include "EmuController.h"

#include "AllHwControllers.h"
#include "logging.h"

#include "Fei4Emu.h"

namespace {
    auto logger = logging::make_log("fei4_emu_controller");
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

template<>
void EmuController<Fei4, Fei4Emu>::loadConfig(const json &j) {
//    EmuTxCore::setCom(new EmuShm(j["tx"]["id"], j["tx"]["size"], true));
//    EmuRxCore::setCom(new EmuShm(j["rx"]["id"], j["rx"]["size"], true));

  if (j.contains("rxWaitTime")) {
    m_waitTime = std::chrono::microseconds(j["rxWaitTime"]);
  }

  // Tx EmuCom
  tx_coms.emplace_back(new RingBuffer());
  EmuTxCore<Fei4>::setCom(0, tx_coms.back().get());
  // Rx EmuCom
  rx_coms.emplace_back(new RingBuffer());
  EmuRxCore<Fei4>::setCom(0, rx_coms.back().get());

  auto tx = EmuTxCore<Fei4>::getCom(0);
  auto rx = EmuRxCore<Fei4>::getCom(0);

  //TODO make nice
  logger->info("Starting FEI4 Emulator");
  const json &emuCfg = j["__feCfg_data__"];
  emus.emplace_back(new Fei4Emu(emuCfg, rx, tx));
  emuThreads.push_back(std::thread(&Fei4Emu::executeLoop, emus.back().get()));
}
