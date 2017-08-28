#ifndef SCANFACTORY_H
#define SCANFACTORY_H

// #################################
// # Author: Timon Heim
// # Email: timon.heim at cern.ch
// # Project: Yarr
// # Description: Scan Factory
// # Comment: Depends on dictionary for FE
// ################################

#include "ScanBase.h"
#include "json.hpp"

#include "AllStdActions.h"
#include "AllFei4Actions.h"

using json=nlohmann::basic_json<std::map, std::vector, std::string, bool, std::int32_t, std::uint32_t, float>;

class ScanFactory : public ScanBase {
    public:
        ScanFactory(Bookkeeper *k);

        void loadConfig(json &scanCfg);

        void init();
        void preScan();
        void postScan();
    private:
        json m_config;
};

#endif
