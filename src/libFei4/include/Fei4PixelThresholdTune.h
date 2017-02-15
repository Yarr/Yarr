#ifndef FEI4PIXELTHRESHOLDTUNE_H
#define FEI4PIXELTHRESHOLDTUNE_H

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

class Fei4PixelThresholdTune : public ScanBase {
    public:
        Fei4PixelThresholdTune(Bookkeeper *k);
        void init();
        void preScan();
        void postScan() {}

		Bookkeeper *keeper;

    private:
        enum MASK_STAGE mask;
        enum DC_MODE dcMode;
        unsigned numOfTriggers;
        double triggerFrequency;
        unsigned triggerDelay;
        bool useScap;
        bool useLcap;

        double target;
        bool verbose;
};

#endif
