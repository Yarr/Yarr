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
    ChipEmu *emu;
    std::vector<std::thread> emuThreads;

    public:
        EmuController(RingBuffer * rx, RingBuffer * tx);
        ~EmuController();
        void loadConfig(json &j);
};

template<class FE, class ChipEmu>
EmuController<FE, ChipEmu>::EmuController(RingBuffer * rx, RingBuffer * tx)
  : emu(nullptr) {
    EmuTxCore<FE>::setCom(tx);
    EmuRxCore<FE>::setCom(rx);
}

template<class FE, class ChipEmu>
EmuController<FE, ChipEmu>::~EmuController() {
  if(emu) {
    emu->run = false;
  }
  emuThreads[0].join();
  if(emu) {
    delete emu;
  }

  auto tx = EmuTxCore<FE>::getCom();
  auto rx = EmuRxCore<FE>::getCom();
  delete rx;
  delete tx;
}




#endif
