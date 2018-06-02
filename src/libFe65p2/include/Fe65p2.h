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
        Fe65p2();
        Fe65p2(TxCore *arg_core);
        Fe65p2(TxCore *arg_core, unsigned arg_channel);
        Fe65p2(TxCore *arg_core, unsigned arg_txChannel, unsigned arg_rxChannel);
        ~Fe65p2() {}

        void init(TxCore *arg_core, unsigned arg_txChannel, unsigned arg_rxChannel) override;

        void configure() override;
        void configureInit();
        void configureGlobal();
        void configurePixels();
        void configDac();

        void maskPixel(unsigned col, unsigned row) override {
            this->setPixConf(col+1, row+1, 0x0);
        }

        void writeNamedRegister(std::string name, uint16_t value) override;
        
        void setInjCharge(double charge, bool sCap=true, bool lCap=true) override final {
            this->setValue(&Fe65p2GlobalCfg::PlsrDac, this->toVcal(charge));
            this->configDac();
        }


    private:
};
#endif
