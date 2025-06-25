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

/// Build a scan based on json configuration.
class ScanFactory : public ScanBase {
    public:
        /** Create scan */
        ScanFactory(Bookkeeper *k, FeedbackClipboardMap *fb);

        /** Build scan from json object */
        void loadConfig(const json &scanCfg) override;

        /** Do setup */
        void init() override;
        /** Called before running the scan */
        void preScan() override;
        /** Called after running the scan */
        void postScan() override;
    private:
        json m_config;
        // Keep around until configuration
        FeedbackClipboardMap *feedback;
};

#endif
