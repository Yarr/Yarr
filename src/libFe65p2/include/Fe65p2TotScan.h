// #################################
// # Author: Benjamin Nachman and Timon Heim
// # Email: bnachman / timon.heim at cern.ch
// # Project: Yarr
// # Description: Fe65p2 TOT Scan
// # Data: 2016-August-17
// # Comment: TOT for a known charge  
// ################################

#ifndef FE65P2TOTSCAN_H
#define FE65P2TOTSCAN_H

#include "ScanBase.h"

#include "AllStdActions.h"
#include "AllFe65p2Actions.h"

class Fe65p2TotScan : public ScanBase {
    public:
        Fe65p2TotScan(Bookkeeper *k);

        void init();
        void preScan();
        void postScan() {}
    private:
};

#endif
