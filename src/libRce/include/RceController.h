#ifndef RCECONTROLLER_H
#define RCECONTROLLER_H


#include "RceTxCore.h"
#include "RceRxCore.h"
#include "HwController.h"
#include "json.hpp"

using json=nlohmann::basic_json<std::map, std::vector, std::string, bool, std::int32_t, std::uint32_t, float>;

class RceController : public HwController, public RceTxCore, public RceRxCore {
    public:
        RceController() {} 
        void loadConfig(json &j);
};

#endif
