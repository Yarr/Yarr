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

/**
 * Base class for top-level implementation of a scan.
 *
 * This is implemented as a series of nested loops.
 * The outer loop is given the index 0.
 */
class ScanBase : public ScanLoopInfo {
    public:
        /** Setup scan using system described by Bookkeeper */
        ScanBase(Bookkeeper *k);
        /** Destroy scan */
        virtual ~ScanBase() = default;

        /** Initialisation */
        virtual void init() {}
        /** Called after initialisation */
        virtual void preScan() {}
        /** Called after scan has finished running */
        virtual void postScan() {}
        /** Run scan control loop */
        void run();

        /// Return non-owning pointer to loop action
        const LoopActionBaseInfo *getLoop(unsigned n) const override;
        /** Return number of levels in this scan */
        unsigned size() const override;
        
        /** Configure scan from json */
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
