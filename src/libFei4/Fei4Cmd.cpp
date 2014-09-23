// #################################
// # Author: Timon Heim
// # Email: timon.heim at cern.ch
// # Project: Yarr
// # Description: FE-I4 Library
// # Comment: Collection of FE-I4 commands
// ################################

#include "Fei4Cmd.h"

Fei4Cmd::Fei4Cmd(TxCore *core, unsigned channel) {
    link = new TxLink(core, channel);
}

Fei4Cmd::~Fei4Cmd() {
    delete link;
}

void Fei4Cmd::trigger() {
    if (verbose) std::cout << __PRETTY_FUNCTION__ << std::endl;
    link->write(0x1D00);
}

void Fei4Cmd::bcr() {
    if (verbose) std::cout << __PRETTY_FUNCTION__ << std::endl;
    link->write(0x1610);
}

void Fei4Cmd::ecr() {
    if (verbose) std::cout << __PRETTY_FUNCTION__ << std::endl;
    link->write(0x1620);
}

void Fei4Cmd::cal() {
    if (verbose) std::cout << __PRETTY_FUNCTION__ << std::endl;
    link->write(0x1640);
}

void Fei4Cmd::wrRegister(int chipId, int address, int value) {
    if (verbose) std::cout << __PRETTY_FUNCTION__ << std::endl;
    link->write(0x005A0800+((chipId<<6)&0x3C0)+(address&0x3F));
    link->write((value<<16)&0xFFFF0000);
}

void Fei4Cmd::rdRegister(int chipId, int address) {
    if (verbose) std::cout << __PRETTY_FUNCTION__ << std::endl;
    link->write(0x005A0400+((chipId<<6)&0x3C0)+(address&0x3F));
}

void Fei4Cmd::wrFrontEnd(int chipId, uint32_t *bitstream) {
    if (verbose) std::cout << __PRETTY_FUNCTION__ << std::endl;
    link->write(0x005A1000+((chipId<<6)&0x3C0));
    //Flipping the order in order to send bit 671-0, and not bit 31-0, 63-21, etc.
    for(int i = 20 ; i >= 0 ; --i) {
        link->write(bitstream[i]);	
    }
}

void Fei4Cmd::runMode(int chipId, bool mode) {
    if (verbose) std::cout << __PRETTY_FUNCTION__ << std::endl;
    uint32_t modeBits = mode ? 0x38 : 0x7;
    link->write(0x005A2800+((chipId<<6)&0x3C0)+modeBits);
}

void Fei4Cmd::globalReset(int chipId) {
    if (verbose) std::cout << __PRETTY_FUNCTION__ << std::endl;
    link->write(0x005A2000+((chipId<<6)&0x3C0));
}

void Fei4Cmd::calTrigger(int delay) {
    if (verbose) std::cout << __PRETTY_FUNCTION__ << std::endl;
    link->write(0x00001640);
    for (int i = 0; i<delay/32; i++){
        link->write(0x00000000);
    }
    link->write(0x1D000000>>delay%32);
}
