#ifndef RD53BCMD_H
#define RD53BCMD_H

// #################################
// # Author: Timon Heim
// # Email: timon.heim at cern.ch
// # Project: Yarr
// # Description: RD53B Library
// # Comment: Collection of RD53B commands
// # Date: May 2020
// ################################

#include "TxCore.h"

class Rd53bCmd {
    public:
        Rd53bCmd();
    protected:
    private:
        TxCore *core;
};

#endif
