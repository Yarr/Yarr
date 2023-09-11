// #################################
// # Author: Timon Heim
// # Email: timon.heim at cern.ch
// # Project: Yarr
// # Description: ITkPixV2 Pixel config class
// # Date: Jul 2023
// ################################

#include "Itkpixv2PixelCfg.h"
#include "logging.h"

namespace {
    auto logger = logging::make_log("Itkpixv2PixelCfg");
}

Itkpixv2PixelCfg::Itkpixv2PixelCfg() {
    for (unsigned dc=0; dc<n_DC; dc++) {
        for (unsigned row=0; row<n_Row; row++) {
            pixRegs[dc][row] = 0x00; //En=1,Inj=0,HitOr=1,TDAC=0
        }
    }
}

void Itkpixv2PixelCfg::setReg(unsigned col, unsigned row, unsigned en, unsigned injen, unsigned hitbus, int tdac) {
    Itkpixv2PixelCfg::pixelBits reg;
    reg.s.en = en;
    reg.s.injen = injen;
    reg.s.hitbus = hitbus;
    reg.s.tdac = abs(tdac);
    reg.s.sign = (tdac < 0) ? 0x1 : 0x0;
    pixRegs[col/2][row] = pixRegs[col/2][row] & (0xFF << (((col+1)&0x1)*8));
    pixRegs[col/2][row] |= ((0xFF & reg.u8) << ((col&0x1)*8));
}

void Itkpixv2PixelCfg::setBit(uint16_t &in, uint8_t bit, uint8_t val) {
    in = (in & ~(1U << bit)) | (val << bit);
}

uint16_t Itkpixv2PixelCfg::getBit(uint16_t in, uint8_t bit) {
    return (in >> bit) & 0x1;
}

void Itkpixv2PixelCfg::setEn(unsigned col, unsigned row, unsigned v) {
    setBit(pixRegs[col / 2][row], (col & 0x1) * 8 + 0, v);
}

void Itkpixv2PixelCfg::setInjEn(unsigned col, unsigned row, unsigned v) {
    setBit(pixRegs[col/2][row], (col&0x1)*8 + 1, v);
}

void Itkpixv2PixelCfg::setHitbus(unsigned col, unsigned row, unsigned v) {
    setBit(pixRegs[col/2][row], (col&0x1)*8 + 2, v);
}

void Itkpixv2PixelCfg::setTDAC(unsigned col, unsigned row, int v) {
    Itkpixv2PixelCfg::pixelBits reg;
    reg.u8 = (pixRegs[col/2][row] >> ((col&0x1)*8)) & 0xFF;
    reg.s.tdac = abs(v);
    reg.s.sign = (v < 0) ? 0x1 : 0x0;
    // Clear old regs and set new ones
    pixRegs[col/2][row] = (pixRegs[col/2][row] & ~(0xFF << ((col&0x1)*8))) | (reg.u8 << ((col&0x1)*8));
}   

unsigned Itkpixv2PixelCfg::getEn(unsigned col, unsigned row) {
   return getPixelBit(pixRegs, col, row, 0);
}

unsigned Itkpixv2PixelCfg::getInjEn(unsigned col, unsigned row) {
   return getPixelBit(pixRegs, col, row, 1);
}

unsigned Itkpixv2PixelCfg::getHitbus(unsigned col, unsigned row) {
   return getPixelBit(pixRegs, col, row, 2);
}

int Itkpixv2PixelCfg::getTDAC(unsigned col, unsigned row) {
    Itkpixv2PixelCfg::pixelBits reg;
    reg.u8 = (pixRegs[col/2][row] >> ((col&0x1)*8)) & 0xFF;
    return ((int)reg.s.tdac * (reg.s.sign == 0 ? +1 : -1));
}

void Itkpixv2PixelCfg::writeConfig(json &j) {
    for (unsigned col=0; col<n_Col; col++) {
        for (unsigned row=0; row<n_Row; row++) {
            j["ITKPIXV2"]["PixelConfig"][col]["Col"] = col;
            j["ITKPIXV2"]["PixelConfig"][col]["Enable"][row] = this->getEn(col, row);
            j["ITKPIXV2"]["PixelConfig"][col]["Hitbus"][row] = this->getHitbus(col, row);
            j["ITKPIXV2"]["PixelConfig"][col]["InjEn"][row] = this->getInjEn(col, row);
            j["ITKPIXV2"]["PixelConfig"][col]["TDAC"][row] = this->getTDAC(col, row);
        }
    }
}

// TODO add failsaife
void Itkpixv2PixelCfg::loadConfig(const json &j) {
    if (j.contains({"ITKPIXV2","PixelConfig"})) {
		for (unsigned col=0; col<n_Col; col++) {
			for (unsigned row=0; row<n_Row; row++) {
				this->setEn(col, row, j["ITKPIXV2"]["PixelConfig"][col]["Enable"][row]);
				this->setHitbus(col, row, j["ITKPIXV2"]["PixelConfig"][col]["Hitbus"][row]);
				this->setInjEn(col, row, j["ITKPIXV2"]["PixelConfig"][col]["InjEn"][row]);
				this->setTDAC(col, row, j["ITKPIXV2"]["PixelConfig"][col]["TDAC"][row]);
			}
		}
	} else {
		logger->error("Could not find pixel registers, using default!");
	}
}

uint16_t Itkpixv2PixelCfg::getPixelBit(PixelArray &input, unsigned col, unsigned row, unsigned bit){
    return getBit(input[col/2][row], (col&0x1)*8 + bit);
}

uint16_t Itkpixv2PixelCfg::toTenBitMask(uint16_t pixReg) {
    return uint16_t(0x3FF & (((pixReg&0x700)>>3) | (pixReg&0x7)));
}
