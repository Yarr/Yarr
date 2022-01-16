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

#include "FeedbackBase.h"
#include "storage.hpp"
#include <functional>
class ScanFactory : public ScanBase {
    public:
        ScanFactory(Bookkeeper *k, FeedbackClipboardMap *fb);

        void loadConfig(const json &scanCfg);

        void init();
        void preScan();
        void postScan();
    private:
        json m_config;
        // Keep around until configuration
        FeedbackClipboardMap *feedback;
};

namespace StdDict {
    bool registerScan(std::string name,
                      std::function<std::unique_ptr<ScanBase>(Bookkeeper *k)> f);
    std::unique_ptr<ScanBase> getScan(std::string name, Bookkeeper *k);

    std::vector<std::string> listScans();
}

#endif
