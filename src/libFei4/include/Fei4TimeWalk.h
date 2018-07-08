#ifndef FEI4TIMEWALK_H
#define FEI4TIMEWALK_H

// #################################
// # Author: Eunchong Kim
// # Email: eunchong.kim at cern.ch
// # Project: masspro on Yarr
// # Description: Fei4 Time Walk Scan
// # Comment: 
// ################################

#include "ScanBase.h"

#include "AllStdActions.h"
#include "AllFei4Actions.h"

class Fei4TimeWalk : public ScanBase {
    public:
        Fei4TimeWalk(Bookkeeper *b);
        
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
        unsigned minVcal;
        unsigned maxVcal;
        unsigned stepVcal;

        bool verbose;
};

#endif
