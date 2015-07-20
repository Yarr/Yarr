#ifndef FEI4GLOBALTHRESHOLDTUNE_H
#define FEI4GLOBALTHRESHOLDTUNE_H

// #################################
// # Author: Timon Heim
// # Email: timon.heim at cern.ch
// # Project: Yarr
// # Description: Fei4 Threshold Scan
// # Comment: Nothing fancy
// ################################

#include "ScanBase.h"

#include "AllStdActions.h"
#include "AllFei4Actions.h"

class Fei4GlobalThresholdTune : public ScanBase {
    public:
        Fei4GlobalThresholdTune(Fei4 *fe, TxCore *tx, RxCore *rx, ClipBoard<RawDataContainer> *data);
		Fei4GlobalThresholdTune(Bookkeeper *k);
        void init();
        void preScan();
        void postScan() {}
    private:
        enum MASK_STAGE mask;
        enum DC_MODE dcMode;
        unsigned numOfTriggers;
        double triggerFrequency;
        unsigned triggerDelay;
        unsigned minVcal;
        unsigned maxVcal;
        unsigned stepVcal;
        bool useScap;
        bool useLcap;

        double target;
        bool verbose;
};

#endif
