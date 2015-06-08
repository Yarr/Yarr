#ifndef BOOKKEEPER_H
#define BOOKKEEPER_H

// #################################
// # Author: Timon Heim
// # Email: timon.heim at cern.ch
// # Project: Yarr
// # Description: Bookkeeper
// # Comment: Global container for data
// ################################

#include <iostream>
#include <mutex>
#include <deque>

#include "RawData.h"
#include "Fei4EventData.h"
#include "HistogramBase.h"
#include "ResultBase.h"
#include "ClipBoard.h"


#include "Fei4.h"
#include "TxCore.h"
#include "RxCore.h"

class ScanBase;

class Bookkeeper {
    public:
        Bookkeeper();
        ~Bookkeeper();

        Fei4 *fe;
        Fei4 *g_fe;
        TxCore *tx;
        RxCore *rx;
        ClipBoard<RawDataContainer> *data;

        ScanBase *s;
    private:
        


};

#endif
