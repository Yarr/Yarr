#ifndef RCECONTROLLER_H
#define RCECONTROLLER_H


#include "RceTxCore.h"
#include "RceRxCore.h"
#include "HwController.h"


#include "storage.hpp"

class RceController : public HwController, public RceTxCore, public RceRxCore {
    public:
        RceController() {} 
        void loadConfig(json &j);
};

#endif
