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

#include "Fei4.h"
#include "TxCore.h"
#include "RxCore.h"
#include "LoopEngine.h"
#include "LoopActionBase.h"
#include "ClipBoard.h"
#include "RawData.h"

#include "Bookkeeper.h"

class ScanBase {
    public:
        ScanBase(Fei4 *fe, TxCore *tx, RxCore *rx, ClipBoard<RawDataContainer> *data);
        ScanBase(Bookkeeper *k);
        virtual ~ScanBase() {}

        virtual void init() {}
        virtual void preScan() {}
        virtual void postScan() {}
        void run();

        std::shared_ptr<LoopActionBase> getLoop(unsigned n);
        std::shared_ptr<LoopActionBase> operator[](unsigned n);
        std::shared_ptr<LoopActionBase> operator[](std::type_index t);
        unsigned size();

    protected:
        LoopEngine engine;
        void addLoop(std::shared_ptr<LoopActionBase> l);
        Bookkeeper *b;

        Fei4 *g_fe;
        TxCore *g_tx;
        RxCore *g_rx;
        ClipBoard<RawDataContainer> *g_data;

    private:
        std::vector<std::shared_ptr<LoopActionBase> > loops;
        std::map<std::type_index, std::shared_ptr<LoopActionBase> > loopMap;
};

#endif
