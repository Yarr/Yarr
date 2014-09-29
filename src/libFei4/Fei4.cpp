// #################################
// # Author: Timon Heim
// # Email: timon.heim at cern.ch
// # Project: Yarr
// # Description: FE-I4 Library
// # Comment: FEI4 Base class
// ################################

#include "Fei4.h"

Fei4::Fei4(TxCore *core, unsigned arg_chipId) : Fei4GlobalCfg(), Fei4PixelCfg(), Fei4Cmd(core) {
    chipId = arg_chipId;
}

void Fei4::configure() {
    runMode(chipId, false);
    
    // Increase threshold
    uint16_t tmp = getValue(&Fei4::Vthin_Coarse);
    writeRegister(&Fei4::Vthin_Coarse, 255);

    for (unsigned i=0; i<numRegs; i++) {
        wrRegister(chipId, i, cfg[i]);
    }

    // Set actual threshold
    setValue(&Fei4::Vthin_Coarse, tmp);
    writeRegister(&Fei4::Vthin_Coarse);

}

void Fei4::configurePixels() {
    writeRegister(&Fei4::Colpr_Mode, 0x0);
    for (unsigned dc=0; dc<Fei4PixelCfg::n_DC; dc++) {
        writeRegister(&Fei4::Colpr_Addr, dc);
        for (unsigned bit=0; bit<Fei4PixelCfg::n_Bits; bit++) {
            wrFrontEnd(chipId, getCfg(bit, dc));
            loadIntoPixel(bit);
        }
    }
}

void Fei4::initMask(enum MASK_STAGE mask) {
    uint32_t bitstream[21];
    for(unsigned i=0; i<21; i++)
        bitstream[i] = mask;
    wrFrontEnd(chipId, bitstream);
}

void Fei4::initMask(uint32_t mask) {
    uint32_t bitstream[21];
    for(unsigned i=0; i<21; i++)
        bitstream[i] = mask;
    wrFrontEnd(chipId, bitstream);
}

void Fei4::shiftMask() {
    this->loadIntoShiftReg(0x1);
    this->loadIntoPixel(0x1);
    this->loadIntoShiftReg(0x1);
    this->shiftByOne();
}

// Inverts pixel latch
void Fei4::loadIntoShiftReg(unsigned pixel_latch) {
    // Select Pixel latch to copy into SR
    writeRegister(&Fei4::Pixel_latch_strobe, pixel_latch);
    // Select SR in Parallel Input Mode
    writeRegister(&Fei4::S1, 0x1);
    writeRegister(&Fei4::S0, 0x1);
    writeRegister(&Fei4::SR_Clock, 0x1);

    // Copy from Latches into SR
    globalPulse(chipId, 10);

    // Reset SR regs
    writeRegister(&Fei4::S1, 0x0);
    writeRegister(&Fei4::S0, 0x0);
    writeRegister(&Fei4::SR_Clock, 0x0);
    writeRegister(&Fei4::Pixel_latch_strobe, 0x0);
}

void Fei4::loadIntoPixel(unsigned pixel_latch) {
    // Select Pixel latch to copy into SR
    writeRegister(&Fei4::Pixel_latch_strobe, pixel_latch);
    
    // Enable Latches
    writeRegister(&Fei4::Latch_Enable, 0x1);

    // Copy from SR into Latches
    globalPulse(chipId, 10);

    // Reset SR regs
    writeRegister(&Fei4::Latch_Enable, 0x0);
    writeRegister(&Fei4::Pixel_latch_strobe, 0x0);
}

void Fei4::shiftByOne() {
    // Normal Shift Mode
    writeRegister(&Fei4::S1, 0x0);
    writeRegister(&Fei4::S0, 0x0);
    writeRegister(&Fei4::SR_Clock, 0x1);

    // Shift By One Clock Cycle
    globalPulse(chipId, 10);

    writeRegister(&Fei4::SR_Clock, 0x0);
}
