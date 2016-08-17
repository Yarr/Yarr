#ifndef FE65P2NOISESCAN_H
#define FE65P2NOISESCAN_H

// #################################
// # Author: Timon Heim
// # Email: timon.heim at cern.ch
// # Project: Yarr
// # Description: Fe65p2 Noise Scan
// # Comment: Nothing fancy
// ################################

#include "ScanBase.h"

#include "AllStdActions.h"
#include "AllFe65p2Actions.h"

class Fe65p2NoiseScan : public ScanBase {
    public:
        Fe65p2NoiseScan(Bookkeeper *k);
        
        void init();
        void preScan();
        void postScan() {}

    private:
        double triggerFrequency;
        unsigned triggerTime; 
        bool verbose;
};

#endif
