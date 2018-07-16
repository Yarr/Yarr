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
#include "json.hpp"

using json=nlohmann::basic_json<std::map, std::vector, std::string, bool, int32_t, uint32_t, float>;

class HwController : virtual public TxCore, virtual public RxCore {
    public:
        virtual void loadConfig(json &j) = 0 ;
        
        virtual void setupMode() {}
        virtual void runMode() {}

        virtual ~HwController() {}
};

#endif
