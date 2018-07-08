#ifndef FEI4CHARGEVSTOT_H
#define FEI4CHARGEVSTOT_H

// #################################
// # Author: Eunchong Kim
// # Email: eunchong.kim at cern.ch
// # Project: masspro on Yarr
// # Description: Fei4 Charge vs Tot Scan
// # Comment: Nothing fancy
// ################################

#include "ScanBase.h"

#include "AllStdActions.h"
#include "AllFei4Actions.h"

class Fei4ChargeVsTot : public ScanBase {
    public:
        Fei4ChargeVsTot(Bookkeeper *b);
        
        void init();
        void preScan();
        void postScan() {}

    private:
        enum MASK_STAGE mask;
        enum DC_MODE dcMode;
        unsigned numOfTriggers;
        double triggerFrequency;
        unsigned triggerDelay;
        bool useScap;
        bool useLcap;
        double targetCharge;
        unsigned minVcal;
        unsigned maxVcal;
        unsigned stepVcal;

        bool verbose;
};

#endif
