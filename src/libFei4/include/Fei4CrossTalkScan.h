#ifndef FEI4CROSSTALKSCAN_H
#define FEI4CROSSTALKSCAN_H

// ######################################################
// # Author: Jonathan Debus
// # Email: etphysik dot jd at gmail dot com
// # Project: Yarr
// # Description: Fei4 Cross Talk Scan
// # Comment: Prototype, to be tested, not for use yet!
// ######################################################

#include "ScanBase.h"

#include "AllStdActions.h"
#include "AllFei4Actions.h"

class Fei4CrossTalkScan : public ScanBase {
    public:
        Fei4CrossTalkScan(Fei4 *fe, TxCore *tx, RxCore *rx, ClipBoard<RawDataContainer> *data);
        Fei4CrossTalkScan(Bookkeeper *k);

        void init();
        void preScan();
        void postScan() {}

    private:
        enum MASK_STAGE mask;
        enum DC_MODE dcMode;
        unsigned numOfTriggers;
        double triggerFrequency;
        unsigned triggerDelay;

        bool verbose;
};

#endif
