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


#include "storage.hpp"

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

namespace StdDict {
    bool registerScan(std::string name,
                      std::function<std::unique_ptr<ScanBase>(Bookkeeper *k)> f);
    std::unique_ptr<ScanBase> getScan(std::string name, Bookkeeper *k);

    std::vector<std::string> listScans();
}

#endif
