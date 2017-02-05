#ifndef SPECCONTROLLER_H
#define SPECCONTROLLER_H

// #################################
// # Author: Timon Heim
// # Email: timon.heim at cern.ch
// # Project: Yarr
// # Description: Spec Controller class
// # Comment:
// # Data: Feb 2017
// ################################

#include "HwController.h"
#include "SpecTxCore.h"
#include "SpecRxCore.h"

class SpecController : public HwController, public SpecTxCore, public SpecRxCore {
    public:
};

#endif
