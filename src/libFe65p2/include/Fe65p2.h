#ifndef FE65P2_H
#define FE65P2_H

// #################################
// # Author: Timon Heim
// # Email: timon.heim at cern.ch
// # Project: Yarr
// # Description: FE65-p2 Library
// # Comment: FE65-p2 Base class
// ################################

#include "FrontEnd.h"
#include "TxCore.h"
#include "Fe65p2Cfg.h"
#include "Fe65p2Cmd.h"
#include "Fei4EventData.h"
#include "ClipBoard.h"
#include "HistogramBase.h"

class Fe65p2 : public FrontEnd, public Fe65p2Cfg, public Fe65p2Cmd {
    public:
        Fe65p2(TxCore *arg_core);
        Fe65p2(TxCore *arg_core, unsigned arg_channel);
        Fe65p2(TxCore *arg_core, unsigned arg_txChannel, unsigned arg_rxChannel);
        ~Fe65p2() {}

        void configure() override;
        void init();
        void configureGlobal();
        void configurePixels();
        void configDac();

        void writeNamedRegister(std::string name, uint16_t value) override;

    private:
};
#endif
