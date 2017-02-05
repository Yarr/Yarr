#ifndef HWCONTROLLER_H
#define HWCONTROLLER_H

// #################################
// # Author: Timon Heim
// # Email: timon.heim at cern.ch
// # Project: Yarr
// # Description: Abstract hardware controller
// # Date: Feb 2017
// ################################

#include "TxCore.h"
#include "RxCore.h"

class HwController : virtual public TxCore, virtual public RxCore {
    public:
        virtual void loadConfig() {} //TODO make pure virtual
};

#endif
