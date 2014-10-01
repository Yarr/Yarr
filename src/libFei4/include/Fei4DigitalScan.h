#ifndef FEI4DIGITALSCAN_H
#define FEI4DIGITALSCAN_H

// #################################
// # Author: Timon Heim
// # Email: timon.heim at cern.ch
// # Project: Yarr
// # Description: Fei4 Digital Scan
// # Comment: Nothing fancy
// ################################

#include "ScanBase.h"

#include "AllStdActions.h"
#include "AllFei4Actions.h"

class Fei4DigitalScan : public ScanBase {
    public:
        Fei4DigitalScan(Fei4 *fe, TxCore *tx, RxCore *rx, ClipBoard<RawData> *data);
        
        void init();
        void configure();

    private:
        enum MASK_STAGE mask;
        enum DC_MODE dcMode;
        unsigned numOfTriggers;
        double triggerFrequency;
        unsigned triggerDelay;

        bool verbose;
};

#endif
