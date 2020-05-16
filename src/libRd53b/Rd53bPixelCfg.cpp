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
            pixRegs[dc][row] = 0x55; //En=1,Inj=0,HitOr=1,TDAC=0
        }
    }
}

void Rd53bPixelCfg::setEn(unsigned col, unsigned row, unsigned v) {

}

void Rd53bPixelCfg::setHitbus(unsigned col, unsigned row, unsigned v) {

}

void Rd53bPixelCfg::setInjEn(unsigned col, unsigned row, unsigned v) {

}

void Rd53bPixelCfg::setTDAC(unsigned col, unsigned row, int v) {

}


