/*
* Author: Ondra Kovanda, ondrej.kovanda at cern.ch
* Date: 05/2024
* Description: Controller for ITkPixV2 emulator
*/

#include "EmuController.h"
#include "AllHwControllers.h"
#include "Itkpixv2.h"
#include "Itkpixv2Emu.h"
#include "logging.h"

namespace {
    auto logger = logging::make_log("Itkpixv2_emu_controller");
}

//this template is redefined in all emulators, and could be moved to
//EmuController.h to avoid duplication
template<class FE, class ChipEmu>
std::unique_ptr<HwController> makeEmu() {
  auto ctrl = std::make_unique< EmuController<FE, ChipEmu> >();
  return ctrl;
}

//register the controller to the list of available controllers for e. g. ScanHelper
bool emu_registered_Itkpixv2 = StdDict::registerHwController("emu_Itkpixv2", makeEmu<Itkpixv2, Itkpixv2Emu>);

//define the EmuController::loadConfig FE-specific method
template<>
void EmuController<Itkpixv2, Itkpixv2Emu>::loadConfig(const json &j) {
  logger->info("in loadConfig");
  //Leaving this in for the time being - not sure if Itkpixv2 has this; from here
  if (j.contains("rxWaitTime")) {
    m_waitTime = std::chrono::microseconds(j["rxWaitTime"]);
  }

  int srand_seed = time(nullptr);
  std::string infotoken = "";
  if (j.contains("seed") && j["seed"] == "fixed") {
    srand_seed = 1;
    infotoken = " Random Seed Fixed";
  }
  logger->info("in loadConfig 2");
  //till here

  // Tx EmuCom - create the actual pipeline for commands (RingBuffer)
  // and attach the command 'generator' (EmuTxCore) to it. tx_coms and rx_coms
  // are vectors of unique pointers to RingBuffers, which are members of the
  // EmuController class. EmuCom is an alias for RingBuffer. Eventually,
  // these are communicated in the scans through a BookKeeper object to the
  // ScanBase
  tx_coms.emplace_back(new RingBuffer());
  EmuTxCore<Itkpixv2>::setCom(0, tx_coms.back().get());

  // Rx EmuCom - similar as above, but in the oposite direction
  rx_coms.emplace_back(new RingBuffer());
  EmuRxCore<Itkpixv2>::setCom(0, rx_coms.back().get());

  EmuCom* tx = EmuTxCore<Itkpixv2>::getCom(0);
  EmuCom* rx = EmuRxCore<Itkpixv2>::getCom(0);

  //TODO make nice
  logger->info("Starting Itkpixv2 Emulator" + infotoken);

  //There's probably a better way to do this then store the per-pixel threshold/noise
  //config in one huge file. If we want something random but fixed across scans, we can
  //generate in each instance with fixed seed, avoiding this huge file. That way we can
  //also have this scalable to multiple emulated FEs.
  //const json &emuCfg = j["__feCfg_data__"];
  emus.emplace_back(new Itkpixv2Emu(tx, rx, 5));
  emuThreads.push_back(std::thread(&Itkpixv2Emu::executeLoop, emus.back().get()));
}
