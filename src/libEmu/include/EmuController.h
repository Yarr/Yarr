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

    std::unique_ptr<RingBuffer> rx_com;
    std::unique_ptr<RingBuffer> tx_com;
    

    public:
        EmuController(std::unique_ptr<RingBuffer> rx,
                      std::unique_ptr<RingBuffer> tx);
        ~EmuController();
        void loadConfig(json &j);
};

template<class FE, class ChipEmu>
  EmuController<FE, ChipEmu>::EmuController(std::unique_ptr<RingBuffer> rx,
                                            std::unique_ptr<RingBuffer> tx)
  : rx_com(std::move(rx)), tx_com(std::move(tx)) {
    // Don't transfer ownership!
    EmuTxCore<FE>::setCom(tx_com.get());
    EmuRxCore<FE>::setCom(rx_com.get());
}

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
