#include "EmuController.h"

#include "AllHwControllers.h"
#include "logging.h"

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
