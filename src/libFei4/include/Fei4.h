#ifndef FEI4
#define FEI4

// #################################
// # Author: Timon Heim
// # Email: timon.heim at cern.ch
// # Project: Yarr
// # Description: FE-I4 Library
// # Comment: FEI4 Base class
// ################################

#include <iostream>
#include <string>

#include "FrontEnd.h"
#include "TxCore.h"
#include "Fei4Cmd.h"
#include "Fei4Cfg.h"


enum MASK_STAGE {
    MASK_1  = 0xFFFFFFFF,
    MASK_2  = 0x55555555,
    MASK_4  = 0x11111111,
    MASK_8  = 0x01010101,
    MASK_16 = 0x00010001,
    MASK_32 = 0x00000001,
    MASK_NONE    = 0x00000000
};

enum DC_MODE {
    SINGLE_DC = 0x0,
    QUAD_DC = 0x1,
    OCTA_DC = 0x2,
    ALL_DC = 0x3
};

class Fei4 : public Fei4Cfg, public Fei4Cmd, public FrontEnd {
    public:
        Fei4();
        Fei4(TxCore *arg_core);
        Fei4(TxCore *arg_core, unsigned arg_channel);
        Fei4(TxCore *arg_core, unsigned arg_txchannel, unsigned arg_rxchannel);

        void init(TxCore *arg_core, unsigned arg_txChannel, unsigned arg_rxChannel) override final;

        ~Fei4();

        void configure() override final;
        void configureGlobal();
        void configurePixels(unsigned lsb=0, unsigned msb=Fei4PixelCfg::n_Bits);

        void setRunMode(bool mode=true) {
            runMode(chipId, mode);
        }

        void makeGlobal() override final {
            chipId = 8;
        }

        void initMask(enum MASK_STAGE mask);
        void initMask(uint32_t mask);
        void shiftMask();
        void loadIntoShiftReg(unsigned pixel_latch);
        void loadIntoPixel(unsigned pixel_latch);
        void shiftByOne();
        void writeNamedRegister(std::string name, uint16_t value) override final;
        void readPixelRegister(unsigned colpr_addr, unsigned latch);
        void dummyCmd();

        void maskPixel(unsigned col, unsigned row) override {
            this->setEn(col+1, row+1, 0);
            this->setHitbus(col+1, row+1, 1);
        }

        void writeRegister(Fei4Register Fei4GlobalCfg::*ref, uint16_t cfgBits){
            setValue(ref, cfgBits);
            writeRegister(ref);
        }

        void writeRegister(Fei4Register Fei4GlobalCfg::*ref){
            wrRegister(chipId, getAddr(ref), cfg[getAddr(ref)]);
        }

        uint16_t readRegister(Fei4Register Fei4GlobalCfg::*ref){
            return getValue(ref);
        }

        void readRegister(unsigned addr) {
            this->rdRegister(chipId, addr);
        }
        
        void setInjCharge(double charge, bool use_sCap=true, bool use_lCap=true) override final {
            this->writeRegister(&Fei4GlobalCfg::PlsrDAC, this->toVcal(charge, use_sCap, use_lCap));
        }

        void wrGR16(unsigned int mOffset, unsigned int bOffset, unsigned int mask, bool msbRight, uint16_t cfgBits);
    private:

};

#endif
