// #################################
// # Author: Timon Heim
// # Email: timon.heim at cern.ch
// # Project: Yarr
// # Description: FE-I4 Library
// # Comment: FEI4 Base class
// ################################

#include "Fei4.h"

Fei4::Fei4(TxCore *core, unsigned arg_chipId) : Fei4Cfg(arg_chipId), Fei4Cmd(core) {
    txChannel = 99;
    rxChannel = 99;
	histogrammer = NULL;
	ana = NULL;
	active = true;
}

Fei4::Fei4(TxCore *core, unsigned arg_chipId, unsigned arg_channel) : Fei4Cfg(arg_chipId), Fei4Cmd(core) {
	txChannel = arg_channel;
	rxChannel = arg_channel;
	histogrammer = NULL;
	ana = NULL;
	active = true;
}

Fei4::Fei4(TxCore *core, unsigned arg_chipId, unsigned arg_txChannel, unsigned arg_rxChannel) : Fei4Cfg(arg_chipId), Fei4Cmd(core) {
	txChannel = arg_txChannel;
	rxChannel = arg_rxChannel;
	histogrammer = NULL;
	ana = NULL;
	active = true;
}

Fei4::~Fei4() {	

}

void Fei4::configure() {
    runMode(chipId, false);
    
    // Increase threshold
    uint16_t tmp = getValue(&Fei4::Vthin_Coarse);
    writeRegister(&Fei4::Vthin_Coarse, 255);

    for (unsigned i=0; i<numRegs; i++) {
        wrRegister(chipId, i, cfg[i]);
    }
    
    // Request all Service Records
    writeRegister(&Fei4::ReadErrorReq, 0x1);
    globalPulse(chipId, 10);
    writeRegister(&Fei4::ReadErrorReq, 0x0);


    // Set actual threshold
    setValue(&Fei4::Vthin_Coarse, tmp);
    writeRegister(&Fei4::Vthin_Coarse);
}

void Fei4::configurePixels(unsigned lsb, unsigned msb) {
    // Increase threshold
    uint16_t tmp = getValue(&Fei4::Vthin_Coarse);
    writeRegister(&Fei4::Vthin_Coarse, 255);
    
    // Write Pixel Mask
    writeRegister(&Fei4::Colpr_Mode, 0x0);
    for (unsigned dc=0; dc<Fei4PixelCfg::n_DC; dc++) {
        writeRegister(&Fei4::Colpr_Addr, dc);
        for (unsigned bit=lsb; bit<msb; bit++) {
            wrFrontEnd(chipId, getCfg(bit, dc));
            loadIntoPixel(1 << bit);
            while(core->isCmdEmpty() == 0);
        }
    }
    // Set actual threshold
    setValue(&Fei4::Vthin_Coarse, tmp);
    writeRegister(&Fei4::Vthin_Coarse);
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

bool Fei4::isActive() {
	return active;
}

bool Fei4::getActive() {
	return this->active;
}

void Fei4::setActive(bool arg_active) {
	active = arg_active;
}

unsigned Fei4::getChannel() {
	return rxChannel;
}

unsigned Fei4::getRxChannel() {
	return rxChannel;
}

unsigned Fei4::getTxChannel() {
	return txChannel;
}

void Fei4::setChannel(unsigned arg_channel) {
	txChannel = arg_channel;
	rxChannel = arg_channel;
}

void Fei4::setChannel(unsigned arg_txChannel, unsigned arg_rxChannel) {
	txChannel = arg_txChannel;
	rxChannel = arg_rxChannel;
}

