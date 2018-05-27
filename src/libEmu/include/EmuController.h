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
#include "EmuShm.h"
#include "json.hpp"

#include "RingBuffer.h"

using json=nlohmann::basic_json<std::map, std::vector, std::string, bool, std::int32_t, std::uint32_t, float>;

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
}




#endif
