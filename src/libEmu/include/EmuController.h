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

class EmuController : public HwController, public EmuTxCore, public EmuRxCore {
    public:
        EmuController() {} 
        void loadConfig(nlohmann::json &j);
};

#endif
