#ifndef SCANBASE_H
#define SCANBASE_H

// #################################
// # Author: Timon Heim
// # Email: timon.heim at cern.ch
// # Project: Yarr
// # Description: Scan Base Class
// # Comment:
// ################################

#include <vector>
#include <memory>

#include "LoopEngine.h"
#include "LoopActionBase.h"

#include "storage.hpp"

#include "ScanLoopInfo.h"

class Bookkeeper;
class RxCore;
class TxCore;

class ScanBase : public ScanLoopInfo {
    public:
        ScanBase(Bookkeeper *k);
        virtual ~ScanBase() = default;

        virtual void init() {}
        virtual void preScan() {}
        virtual void postScan() {}
        void run();

        /// Return non-owning pointer to loop action
        const LoopActionBaseInfo *getLoop(unsigned n) const override;
        unsigned size() const override;
        
        virtual void loadConfig(const json &cfg) {}

    protected:
        LoopEngine engine;
        void addLoop(std::shared_ptr<LoopActionBase> l);
        TxCore *g_tx;
        RxCore *g_rx;
        Bookkeeper *g_bk;

    private:
        std::vector<std::shared_ptr<LoopActionBase> > loops;
};

#endif
