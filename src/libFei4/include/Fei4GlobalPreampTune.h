#ifndef FEI4GLOBALPREAMPTUNE_H
#define FEI4GLOBALPREAMPTUNE_H

// #################################
// # Author: Timon Heim
// # Email: timon.heim at cern.ch
// # Project: Yarr
// # Description: Fei4 Preamp Scan
// # Comment: Nothing fancy
// ################################

#include "ScanBase.h"

#include "AllStdActions.h"
#include "AllFei4Actions.h"

class Fei4GlobalPreampTune : public ScanBase {
    public:
        Fei4GlobalPreampTune(Bookkeeper *b);
        
        void init() override;
        void preScan() override;
        void postScan() override {}

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
};

#endif
