#ifndef FE65P2EXTTRIGGER_H
#define FE65P2EXTTRIGGER_H

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

class Fe65p2ExtTrigger : public ScanBase {
    public:
        Fe65p2ExtTrigger(Bookkeeper *k);
        
        void init();
        void preScan();
        void postScan() {}

    private:
        double triggerFrequency;
        unsigned triggerTime; 
        bool verbose;
};

#endif
