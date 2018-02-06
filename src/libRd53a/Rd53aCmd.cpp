// #################################
// # Author: Timon Heim
// # Email: timon.heim at cern.ch
// # Project: Yarr
// # Description: RD53A command 
// # Date: Jul 2017
// ################################

#include "Rd53aCmd.h"
#include <fstream>

Rd53aCmd::Rd53aCmd(TxCore *arg_core) {
    verbose = false;
    core = arg_core;
}

Rd53aCmd::~Rd53aCmd() {

}

const uint16_t Rd53aCmd::enc5to8[32] = {0x6A, 0x6C, 0x71, 0x72,
                             0x74, 0x8B, 0x8D, 0x8E,
                             0x93, 0x95, 0x96, 0x99,
                             0x9A, 0x9C, 0xA3, 0xA5,
                             0xA6, 0xA9, 0xAA, 0xAC, 
                             0xB1, 0xB2, 0xB4, 0xC3, 
                             0xC5, 0xC6, 0xC9, 0xCA, 
                             0xCC, 0xD1, 0xD2, 0xD4};

const uint16_t Rd53aCmd::encTrigger[16] = {0x00, 0x2B, 0x2D, 0x2E,
                                           0x33, 0x35, 0x36, 0x39,
                                           0x3A, 0x3C, 0x4B, 0x4D,
                                           0x4E, 0x53, 0x55, 0x56};

uint32_t Rd53aCmd::encode5to8(uint32_t val) {
    return Rd53aCmd::enc5to8[val & 0x1F];
}

void Rd53aCmd::trigger(uint32_t bc, uint32_t tag, uint32_t bc2, uint32_t tag2) {
    uint32_t tmp = 0;
    if (bc>0) {
        tmp += ((Rd53aCmd::encTrigger[0xF & bc] << 24) | (Rd53aCmd::enc5to8[tag & 0x1F] << 16));
    } else {
        tmp += 0x69690000;
    }
    if (bc2 >0 ) {
        tmp += ((Rd53aCmd::encTrigger[0xF & bc2] << 8) | (Rd53aCmd::enc5to8[tag2 & 0x1F]));
    } else {
        tmp += 0x6969;
    }
    core->writeFifo(tmp);
    core->releaseFifo();
}

void Rd53aCmd::ecr() {
    core->writeFifo(0x5a5a6969);
    core->releaseFifo();
}

void Rd53aCmd::bcr() {
    core->writeFifo(0x59596969);
    core->releaseFifo();
}

void Rd53aCmd::globalPulse(uint32_t chipId, uint32_t duration) {
    core->writeFifo(0x5C5C0000 + (Rd53aCmd::encode5to8(chipId<<1)<<8) + Rd53aCmd::encode5to8(duration<<1));
    core->releaseFifo();
}

// {Cal,Cal}{ChipId[3:0],CalEdgeMode,CalEdgeDelay[2:0],CalEdgeWidth[5:4]}{CalEdgeWidth[3:0],CalAuxMode,CalAuxDly[4:0]}
void Rd53aCmd::cal(uint32_t chipId, uint32_t mode, uint32_t delay, uint32_t duration, uint32_t aux_mode, uint32_t aux_delay) {
    core->writeFifo(0x69696363); 
    
    core->writeFifo((Rd53aCmd::encode5to8((chipId<<1)+(mode&0x1)) << 24)
                + (Rd53aCmd::encode5to8((delay<<2)+((duration>>4)&0x3)) << 16)
                + (Rd53aCmd::encode5to8(((duration&0xF)<<1)+(aux_mode&0x1)) << 8)
                + (Rd53aCmd::encode5to8(aux_delay) << 0));
    core->releaseFifo();
}

void Rd53aCmd::wrRegister(uint32_t chipId, uint32_t address, uint16_t value) {
    if (verbose) std::cout << __PRETTY_FUNCTION__ << " : ID(" << chipId << ") ADR(" << address << ") VAL(0x" << std::hex << value << std::dec << ")" << std::endl;
    // Header
    core->writeFifo(0x69696666);
    uint32_t tmp = 0x0;
    // ID[3:0],0 | ADR[8:4]
    tmp += (this->encode5to8((chipId & 0xF) << 1)) << 24;
    tmp += (this->encode5to8((address >> 4) & 0x1F)) << 16;
    // ADR[3:0],VAL[15] | VAL[14:10]
    tmp += (this->encode5to8(((address & 0xF) << 1) + ((value >> 15) & 0x1)) << 8);
    tmp += (this->encode5to8((value >> 10) & 0x1F));
    core->writeFifo(tmp);
    // VAL[9:5] | VAL [4:0]
    tmp = (this->encode5to8((value >> 5) & 0x1F) << 24);
    tmp += (this->encode5to8(value & 0x1F)) << 16;
    tmp += 0x6969;
    core->writeFifo(tmp);
    core->releaseFifo();
}

// TODO optimise this
void Rd53aCmd::wrRegisterBlock(uint32_t chipId, uint32_t address, uint16_t value[6]) {
    // Header
    core->writeFifo(0x69696666);
    uint32_t tmp = 0x0;
    // ID[3:0],0 | ADR[8:4]
    tmp += (this->encode5to8((chipId & 0xF) << 1)) << 24;
    tmp += (this->encode5to8((address >> 4) & 0x1F)) << 16;
    // ADR[3:0],VAL[15] | VAL[14:10]
    tmp += (this->encode5to8((address<<1) + ((value[0] >> 15) & 0x1)) << 8);
    tmp += (this->encode5to8(value[0] >> 10));
    core->writeFifo(tmp);
    // VAL[9:5] | VAL [4:0] | VAL[15:10] | VAL[
    tmp = (this->encode5to8(value[0] >> 5) << 24);
    tmp += (this->encode5to8(value[0]) << 16);
    tmp += (this->encode5to8(value[1] >> 11) << 8);
    tmp += (this->encode5to8(value[1] >> 6) << 0);
    core->writeFifo(tmp);
    tmp = (this->encode5to8(value[1] >> 1) << 24);
    tmp += (this->encode5to8((value[1]<<4)+((value[2]>>12)&0xF)) << 16);
    tmp += (this->encode5to8(value[2]>>7) << 8);
    tmp +=(this->encode5to8(value[2]>>2));
    core->writeFifo(tmp);
    tmp = (this->encode5to8((value[2]<<3)+((value[3]>>13)&0x7)) << 24);
    tmp += (this->encode5to8(value[3]>>8) << 16);
    tmp += (this->encode5to8(value[3]>>3) << 8);
    tmp += (this->encode5to8((value[3]<<2)+((value[4]>>14)&0x3)));
    core->writeFifo(tmp);
    tmp = (this->encode5to8(value[4]>>9) << 24);
    tmp += (this->encode5to8(value[4]>>4) << 16);
    tmp += (this->encode5to8((value[4]<<1)+((value[5]>>15)&0x1)) << 8);
    tmp += (this->encode5to8(value[5]>>10));
    core->writeFifo(tmp);
    tmp = (this->encode5to8(value[5]>>5) << 24);
    tmp += (this->encode5to8(value[5]) << 16);
    tmp += 0x6969;


    core->releaseFifo();

}

void Rd53aCmd::rdRegister(uint32_t chipId, uint32_t address) {

}

