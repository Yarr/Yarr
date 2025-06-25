#ifndef HWCONTROLLER_H
#define HWCONTROLLER_H

// #################################
// # Author: Timon Heim
// # Email: timon.heim at cern.ch
// # Project: Yarr
// # Description: Abstract hardware controller
// # Date: Feb 2017
// ################################

#include <string>

#include "TxCore.h"
#include "RxCore.h"

#include "storage.hpp"

/**
 * Abstract hardware controller.
 *
 * Use TxCore and RxCore.
 */
class HwController : virtual public TxCore, virtual public RxCore {
    public:
        /** Configure controller */
        virtual void loadConfig(const json &j) = 0 ;
        
        virtual void setupMode() {}
        virtual void runMode() {}
        virtual const json getStatus() { return json{}; };

        virtual bool hasFeature(const std::string &feature) { return false; }
  
        ~HwController() override = default;
};

#endif
