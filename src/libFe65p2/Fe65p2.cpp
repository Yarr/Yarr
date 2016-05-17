#include "Fe65p2.h"

Fe65p2::Fe65p2(TxCore *arg_core) : Fe65p2Cmd(arg_core) {

}

void Fe65p2::configure() {
    this->init();
    this->configureGlobal();
    this->configurePixels();
}

void Fe65p2::init() {
    clocksOn();
    reset();
}

void Fe65p2::configureGlobal() {
    writeGlobal(Fe65p2Cfg::cfg);
    //setPlsrDac(Fe65p2Cfg::dacReg);
}

void Fe65p2::configDac() {
    setPlsrDac(Fe65p2Cfg::dacReg);
}

void Fe65p2::configurePixels() {
    // Set threshold high
    uint16_t tmp1 = getValue(&Fe65p2::Vthin1Dac);
    uint16_t tmp2 = getValue(&Fe65p2::Vthin2Dac);
    setValue(&Fe65p2::Vthin1Dac, 255);
    setValue(&Fe65p2::Vthin2Dac, 0);
    // Configure global regs
    configureGlobal();
    // Configure pixel regs
    for(unsigned i=0; i<Fe65p2Cfg::n_QC; i++) {
        setValue(&Fe65p2::ColSrEn, (0x1<<i));
        setValue(&Fe65p2::OneSr, 0);
        configureGlobal();
        for(unsigned bit=0; bit<Fe65p2Cfg::n_Bits; bit++) {
            writePixel(getCfg(bit, i));
            
            switch (bit) {
                case 0:
                    setValue(&Fe65p2::SignLd, 0x1);
                    break;
                case 1:
                    setValue(&Fe65p2::InjEnLd, 0x1);
                    break;
                case 2:
                    setValue(&Fe65p2::TDacLd, 0x1);
                    break;
                case 3:
                    setValue(&Fe65p2::TDacLd, 0x2);
                    break;
                case 4:
                    setValue(&Fe65p2::TDacLd, 0x4);
                    break;
                case 5:
                    setValue(&Fe65p2::TDacLd, 0x8);
                    break;
                case 6:
                    setValue(&Fe65p2::PixConfLd, 0x1);
                    break;
                case 7:
                    setValue(&Fe65p2::PixConfLd, 0x2);
                    break;
                default:
                    break;
            }

            configureGlobal();

            // Unset Shadow registers
            setValue(&Fe65p2::SignLd, 0x0);
            setValue(&Fe65p2::InjEnLd, 0x0);
            setValue(&Fe65p2::TDacLd, 0x0);
            setValue(&Fe65p2::PixConfLd, 0x0);
            configureGlobal();

        }
    }
    // Reset SR
    writePixel((uint16_t) 0x0);
    // Reset threshold
    setValue(&Fe65p2::Vthin1Dac, tmp1);
    setValue(&Fe65p2::Vthin2Dac, tmp2);
    configureGlobal();

}
