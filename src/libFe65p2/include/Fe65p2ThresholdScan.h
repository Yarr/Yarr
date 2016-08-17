// #################################
// # Author: Timon Heim
// # Email: timon.heim at cern.ch
// # Project: Yarr
// # Description: Fe65p2 Threshold Scan
// # Data: 2016-May-16
// # Comment: Nothing fancy
// ################################

#ifndef FE65P2THRESHOLDSCAN_H
#define FE65P2THRESHOLDSCAN_H

#include "ScanBase.h"

#include "AllStdActions.h"
#include "AllFe65p2Actions.h"

class Fe65p2ThresholdScan : public ScanBase {
    public:
        Fe65p2ThresholdScan(Bookkeeper *k);

        void init();
        void preScan();
        void postScan();
};

#endif
