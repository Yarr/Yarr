#ifndef TXLINK_H
#define TXLINK_H

// #################################
// # Author: Timon Heim
// # Email: timon.heim at cern.ch
// # Project: Yarr
// # Description: YARR FW Library
// # Comment: Transmitter Fabric
// ################################

#include <iostream>
#include "TxCore.h"

class TxLink {
    public:
        TxLink(TxCore *arg_core, unsigned arg_channel);
        ~TxLink();

        void write(uint32_t value);

    private:
        TxCore *core;
        unsigned channel;
};

#endif
