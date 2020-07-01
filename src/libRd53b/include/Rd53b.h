#ifndef RD53B_H
#define RD53B_H
// #################################
// # Author: Timon Heim
// # Email: timon.heim at cern.ch
// # Project: Yarr
// # Description: RD53B Library
// # Comment: Combines ITkPixV1 and CROCv1
// # Date: May 2020
// ################################

#include "FrontEnd.h"
#include "TxCore.h"
#include "RxCore.h"

#include "Rd53bCmd.h"
#include "Rd53bCfg.h"

class Rd53b : public FrontEnd, public Rd53bCfg, public Rd53bCmd{
    public:
        Rd53b();
        Rd53b(HwController *arg_core);
        Rd53b(HwController *arg_core, unsigned arg_channel);
        Rd53b(HwController *arg_core, unsigned arg_txchannel, unsigned arg_rxchannel);
        
        void init(HwController *arg_core, unsigned arg_txChannel, unsigned arg_rxChannel) override;
        void makeGlobal() override {m_chipId = 8;}

        void configure() override;
        void configureInit();
        void configureGlobal();
        void configurePixels();

        void maskPixel(unsigned col, unsigned row) override {}

        void writeRegister(Rd53bReg Rd53bGlobalCfg::*ref, uint16_t value);
        void readRegister(Rd53bReg Rd53bGlobalCfg::*ref);
        void writeNamedRegister(std::string name, uint16_t value) override;

        void setInjCharge(double charge, bool sCap=true, bool lCap=true) override {}
    protected:
    private:
};

#endif
