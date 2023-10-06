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
#include <map>
#include <typeindex>

#include "TxCore.h"
#include "RxCore.h"
#include "LoopEngine.h"
#include "LoopActionBase.h"
#include "ClipBoard.h"
#include "RawData.h"


#include "Bookkeeper.h"

#include "storage.hpp"

class ScanBase {
    public:
        ScanBase(Bookkeeper *k);
        virtual ~ScanBase() = default;

        virtual void init() {}
        virtual void preScan() {}
        virtual void postScan() {}
        void run();

        /// Return non-owning pointer to loop action
        const LoopActionBase *getLoop(unsigned n) const;
        unsigned size();
        
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
