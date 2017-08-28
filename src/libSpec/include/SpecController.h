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
#include "json.hpp"

using json=nlohmann::basic_json<std::map, std::vector, std::string, bool, std::int32_t, std::uint32_t, float>;

class SpecController : public HwController, public SpecTxCore, public SpecRxCore {
    public:

        void loadConfig(json &j) {}
};

#endif
