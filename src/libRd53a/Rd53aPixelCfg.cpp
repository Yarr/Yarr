// #################################
// # Author: Timon Heim
// # Email: timon.heim at cern.ch
// # Project: Yarr
// # Description: RD53A config base class
// # Date: Jun 2017
// ################################

#include "Rd53aPixelCfg.h"

struct pixelFields {
    unsigned en : 1;
    unsigned hitbus : 1;
    unsigned injen : 1;
    unsigned tdac : 5;
    unsigned unused: 1;

};

union pixelBits {
    pixelFields s;
    uint8_t u8;
};

Rd53aPixelCfg::Rd53aPixelCfg() {
    for(auto pixReg: pixRegs)
        pixReg = 0x0;
}

uint16_t Rd53aPixelCfg::maskBits(uint16_t val, unsigned mask) {
    return (val & (0xFFFF & (~mask)));
}

// TODO optimise this
void Rd53aPixelCfg::setEn(unsigned col, unsigned row, unsigned v) {
    pixelBits tmp;
    // Get pixel bits to replace
    tmp.u8 = (pixRegs[this->toIndex(col, row)] >> ((col%2)*8)) & 0xFF; 
    // Set bit
    tmp.s.en = v;
    // Save other pixel bits
    pixRegs[this->toIndex(col, row)]  = pixRegs[this->toIndex(col, row)] & (0xFFFF & ~(0xFF<<((col%2)*8)));
    // Write these pixel bits
    pixRegs[this->toIndex(col, row)] += (0xFF & tmp.u8) << ((col%2)*8);
}

void Rd53aPixelCfg::setHitbus(unsigned col, unsigned row, unsigned v) {
    pixelBits tmp;
    tmp.u8 = (pixRegs[this->toIndex(col, row)] >> ((col%2)*8)) & 0xFF; 
    tmp.s.hitbus = v;
    pixRegs[this->toIndex(col, row)]  = pixRegs[this->toIndex(col, row)] & (0xFFFF & ~(0xFF<<((col%2)*8)));
    pixRegs[this->toIndex(col, row)] += (0xFF & tmp.u8) << ((col%2)*8);
}

void Rd53aPixelCfg::setInjEn(unsigned col, unsigned row, unsigned v) {
    pixelBits tmp;
    tmp.u8 = (pixRegs[this->toIndex(col, row)] >> ((col%2)*8)) & 0xFF; 
    tmp.s.injen = v;
    pixRegs[this->toIndex(col, row)]  = pixRegs[this->toIndex(col, row)] & (0xFFFF & ~(0xFF<<((col%2)*8)));
    pixRegs[this->toIndex(col, row)] += (0xFF & tmp.u8) << ((col%2)*8);
}

void Rd53aPixelCfg::setTDAC(unsigned col, unsigned row, unsigned v) {
    pixelBits tmp;
    tmp.u8 = (pixRegs[this->toIndex(col, row)] >> ((col%2)*8)) & 0xFF; 
    tmp.s.tdac = v; // TODO this needs reinterpretation depending on col
    pixRegs[this->toIndex(col, row)]  = pixRegs[this->toIndex(col, row)] & (0xFFFF & ~(0xFF<<((col%2)*8)));
    pixRegs[this->toIndex(col, row)] += (0xFF & tmp.u8) << ((col%2)*8);
}

unsigned Rd53aPixelCfg::getEn(unsigned col, unsigned row) {
    pixelBits tmp;
    tmp.u8 = (pixRegs[this->toIndex(col, row)] >> ((col%2)*8)) & 0xFF;
    return tmp.s.en;
}

unsigned Rd53aPixelCfg::getHitbus(unsigned col, unsigned row) {
    pixelBits tmp;
    tmp.u8 = (pixRegs[this->toIndex(col, row)] >> ((col%2)*8)) & 0xFF;
    return tmp.s.hitbus;
}

unsigned Rd53aPixelCfg::getInjEn(unsigned col, unsigned row) {
    pixelBits tmp;
    tmp.u8 = (pixRegs[this->toIndex(col, row)] >> ((col%2)*8)) & 0xFF;
    return tmp.s.injen;
}

unsigned Rd53aPixelCfg::getTDAC(unsigned col, unsigned row) {
    pixelBits tmp;
    tmp.u8 = (pixRegs[this->toIndex(col, row)] >> ((col%2)*8)) & 0xFF;
    return tmp.s.tdac; // TODO this requires reinterpreation
}

void Rd53aPixelCfg::toFileJson(json &j) {
    for (unsigned col=0; col<n_Col; col++) {
        for (unsigned row=0; row<n_Row; row++) {
            j["RD53A"]["PixelConfig"][col]["Col"] = col;
            j["RD53A"]["PixelConfig"][col]["Enable"][row] = this->getEn(col, row);
            j["RD53A"]["PixelConfig"][col]["Hitbus"][row] = this->getHitbus(col, row);
            j["RD53A"]["PixelConfig"][col]["InjEn"][row] = this->getInjEn(col, row);
            j["RD53A"]["PixelConfig"][col]["TDAC"][row] = this->getTDAC(col, row);
        }
    }
}

// TODO add failsaife
void Rd53aPixelCfg::fromFileJson(json &j) {
    for (unsigned col=0; col<n_Col; col++) {
        for (unsigned row=0; row<n_Row; row++) {
            this->setEn(col, row, j["RD53A"]["PixelConfig"][col]["Enable"][row]);
            this->setHitbus(col, row, j["RD53A"]["PixelConfig"][col]["Hitbus"][row]);
            this->setInjEn(col, row, j["RD53A"]["PixelConfig"][col]["InjEn"][row]);
            this->setTDAC(col, row, j["RD53A"]["PixelConfig"][col]["TDAC"][row]);
        }
    }
}


