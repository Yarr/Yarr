#ifndef RD53A_H
#define RD53A_H


// #################################
// # Author: Timon Heim
// # Email: timon.heim at cern.ch
// # Project: Yarr
// # Comment: RD53A base class
// # Date: Jun 2017
// #################################

#include <iostream>

#include "FrontEnd.h"
#include "TxCore.h"
#include "RxCore.h"
#include "Rd53aCfg.h"
#include "Rd53aCmd.h"

class Rd53a : public FrontEnd, public Rd53aCfg, public Rd53aCmd {
    public:
        Rd53a(TxCore *arg_core);
        Rd53a(TxCore *arg_core, unsigned arg_channel);
        Rd53a(TxCore *arg_core, unsigned arg_txchannel, unsigned arg_rxchannel);

        void configure() {}
        void configureGlobal();
        void configurePixels();
    protected:
    private:
};

#endif
