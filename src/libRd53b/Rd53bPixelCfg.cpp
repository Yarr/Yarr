// #################################
// # Author: Timon Heim
// # Email: timon.heim at cern.ch
// # Project: Yarr
// # Description: RD53B Pixel config class
// # Date: May 2020
// ################################

#include "Rd53bPixelCfg.h"

Rd53bPixelCfg::Rd53bPixelCfg() {
    for (unsigned dc=0; dc<n_DC; dc++) {
        for (unsigned row=0; row<n_Row; row++) {
            pixRegs[dc][row] = 0x00; //En=1,Inj=0,HitOr=1,TDAC=0
        }
    }
}

void Rd53bPixelCfg::setReg(unsigned col, unsigned row, unsigned en, unsigned injen, unsigned hitbus, int tdac) {
    Rd53bPixelCfg::pixelBits reg;
    reg.s.en = en;
    reg.s.injen = injen;
    reg.s.hitbus = hitbus;
    reg.s.tdac = abs(tdac);
    reg.s.sign = (tdac < 0) ? 0x1 : 0x0;
    pixRegs[col/2][row] = pixRegs[col/2][row] & (0xFF << (((col+1)&0x1)*8));
    pixRegs[col/2][row] |= ((0xFF & reg.u8) << ((col&0x1)*8));
}

uint16_t Rd53bPixelCfg::setBit(uint16_t in, uint8_t bit, uint8_t val) {
    return (in & ~(1U << bit)) | (val << bit);
}

uint16_t Rd53bPixelCfg::getBit(uint16_t in, uint8_t bit) {
    return (in >> bit) & 0x1;
}

void Rd53bPixelCfg::setEn(unsigned col, unsigned row, unsigned v) {
    pixRegs[col/2][row] = setBit(pixRegs[col/2][row], (col&0x1)*8 + 0, v);
}

void Rd53bPixelCfg::setInjEn(unsigned col, unsigned row, unsigned v) {
    pixRegs[col/2][row] = setBit(pixRegs[col/2][row], (col&0x1)*8 + 1, v);
}

void Rd53bPixelCfg::setHitbus(unsigned col, unsigned row, unsigned v) {
    pixRegs[col/2][row] = setBit(pixRegs[col/2][row], (col&0x1)*8 + 2, v);
}

void Rd53bPixelCfg::setTDAC(unsigned col, unsigned row, int v) {
    Rd53bPixelCfg::pixelBits reg;
    reg.u8 = (pixRegs[col/2][row] >> ((col&0x1)*8)) & 0xFF;
    reg.s.tdac = abs(v);
    reg.s.sign = (v < 0) ? 0x1 : 0x0;
    // Clear old regs and set new ones
    pixRegs[col/2][row] = (pixRegs[col/2][row] & ~(0xFF << ((col&0x1)*8))) | (reg.u8 << ((col&0x1)*8));
}   

unsigned Rd53bPixelCfg::getEn(unsigned col, unsigned row) {
   return getBit(pixRegs[col/2][row], (col&0x1)*8 + 0);
}

unsigned Rd53bPixelCfg::getInjEn(unsigned col, unsigned row) {
   return getBit(pixRegs[col/2][row], (col&0x1)*8 + 1);
}

unsigned Rd53bPixelCfg::getHitbus(unsigned col, unsigned row) {
   return getBit(pixRegs[col/2][row], (col&0x1)*8 + 2);
}

int Rd53bPixelCfg::getTDAC(unsigned col, unsigned row) {
    Rd53bPixelCfg::pixelBits reg;
    reg.u8 = (pixRegs[col/2][row] >> ((col&0x1)*8)) & 0xFF;
    return (reg.s.tdac*(-1*reg.s.sign));
}

void Rd53bPixelCfg::toJson(json &j) {
    for (unsigned col=0; col<n_Col; col++) {
        for (unsigned row=0; row<n_Row; row++) {
            j["RD53B"]["PixelConfig"][col]["Col"] = col;
            j["RD53B"]["PixelConfig"][col]["Enable"][row] = this->getEn(col, row);
            j["RD53B"]["PixelConfig"][col]["Hitbus"][row] = this->getHitbus(col, row);
            j["RD53B"]["PixelConfig"][col]["InjEn"][row] = this->getInjEn(col, row);
            j["RD53B"]["PixelConfig"][col]["TDAC"][row] = this->getTDAC(col, row);
        }
    }
}

// TODO add failsaife
void Rd53bPixelCfg::fromJson(json &j) {
    for (unsigned col=0; col<n_Col; col++) {
        for (unsigned row=0; row<n_Row; row++) {
            this->setEn(col, row, j["RD53B"]["PixelConfig"][col]["Enable"][row]);
            this->setHitbus(col, row, j["RD53B"]["PixelConfig"][col]["Hitbus"][row]);
            this->setInjEn(col, row, j["RD53B"]["PixelConfig"][col]["InjEn"][row]);
            this->setTDAC(col, row, j["RD53B"]["PixelConfig"][col]["TDAC"][row]);
        }
    }
}
