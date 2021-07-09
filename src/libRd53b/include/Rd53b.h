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

        union EfuseInfo {
            uint32_t data;
            struct {
                unsigned int parity : 8;
                unsigned int chipId : 20;
                unsigned int probe_location : 4;
            } fields;
        };

        Rd53b();
        Rd53b(HwController *arg_core);
        Rd53b(HwController *arg_core, unsigned arg_channel);
        Rd53b(HwController *arg_core, unsigned arg_txchannel, unsigned arg_rxchannel);
        
        void init(HwController *arg_core, unsigned arg_txChannel, unsigned arg_rxChannel) override;
        void makeGlobal() override {m_chipId = 16;}

        void configure() override;
        void configureInit();
        void configureGlobal();
        void configurePixels();
        void configurePixels(std::vector<std::pair<unsigned, unsigned>> &pixels);
        
        int checkCom() override;

        void maskPixel(unsigned col, unsigned row) override {
            this->setEn(col, row, 0);
            this->setHitbus(col, row, 0);
        }

        void enableAll() override;

        void writeRegister(Rd53bReg Rd53bGlobalCfg::*ref, uint16_t value);
        void readRegister(Rd53bReg Rd53bGlobalCfg::*ref);
        void writeNamedRegister(std::string name, uint16_t value) override;
        Rd53bReg Rd53bGlobalCfg::* getNamedRegister(std::string name);

        void setInjCharge(double charge, bool sCap=true, bool lCap=true) override {
            this->writeRegister((Rd53bReg Rd53bGlobalCfg::*)&Rd53bGlobalCfg::InjVcalDiff, this->toVcal(charge));
        }
        
        static std::pair<uint32_t, uint32_t> decodeSingleRegRead(uint32_t higher, uint32_t lower);

        // perform the necessary steps to program the E-fuse circuitry and perform
        // the readback of the E-fuse data
        EfuseInfo readEfuses();

        uint32_t readSingleRegister(Rd53bReg Rd53bGlobalCfg::*ref);
    protected:
    private:
};

#endif
