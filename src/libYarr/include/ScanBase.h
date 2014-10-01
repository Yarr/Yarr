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

#include "Fei4.h"
#include "TxCore.h"
#include "RxCore.h"
#include "LoopEngine.h"
#include "LoopActionBase.h"
#include "ClipBoard.h"
#include "RawData.h"

class ScanBase {
    public:
        ScanBase(Fei4 *fe, TxCore *tx, RxCore *rx, ClipBoard<RawData> *data);

        virtual void init() {}
        virtual void configure() {}
        void run();

        std::shared_ptr<LoopActionBase> getLoop(unsigned n);
        std::shared_ptr<LoopActionBase> operator[](unsigned n);
        unsigned size();

    protected:
        LoopEngine engine;
        std::vector<std::shared_ptr<LoopActionBase> > loops;
        Fei4 *g_fe;
        TxCore *g_tx;
        RxCore *g_rx;
        ClipBoard<RawData> *g_data;


    private:
};

#endif
