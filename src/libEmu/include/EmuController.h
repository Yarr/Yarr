#ifndef EMUCONTROLLER_H
#define EMUCONTROLLER_H

// #################################
// # Author: Timon Heim
// # Email: timon.heim at cern.ch
// # Project: Yarr
// # Description: Emulator Controller
// # Comment:
// # Date: Feb 2017
// ################################

#include "HwController.h"
#include "EmuTxCore.h"
#include "EmuRxCore.h"

#include "RingBuffer.h"

#include "storage.hpp"

class Fei4;
class Rd53a;

template<class FE, class ChipEmu>
class EmuController : public HwController, public EmuTxCore<FE>, public EmuRxCore<FE> {
    std::vector<std::unique_ptr<ChipEmu>> emus;
    std::vector<std::thread> emuThreads;

    std::vector<std::unique_ptr<RingBuffer>> rx_coms;
    std::vector<std::unique_ptr<RingBuffer>> tx_coms;

    public:
        EmuController() = default;
        ~EmuController() override;
        void loadConfig(const json &j) override;
};

template<class FE, class ChipEmu>
EmuController<FE, ChipEmu>::~EmuController() {
  for (auto& emu : emus) {
    emu->run = false;
  }
  for (auto& thread : emuThreads) {
    thread.join();
  }
}




#endif
