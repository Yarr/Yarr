// #################################
// # Author: Timon Heim
// # Email: timon.heim at cern.ch
// # Project: Yarr
// # Description: Fe65p2 Analog Scan
// # Data: 2016-Mar-30
// # Comment: Nothing fancy
// ################################

#ifndef FE65P2ANALOGSCAN_H
#define FE65P2ANALOGSCAN_H

#include "ScanBase.h"

#include "AllStdActions.h"
#include "AllFe65p2Actions.h"

class Fe65p2AnalogScan : public ScanBase {
    public:
        Fe65p2AnalogScan(Bookkeeper *k);

        void init();
        void preScan();
        void postScan() {}
    private:
};

#endif
