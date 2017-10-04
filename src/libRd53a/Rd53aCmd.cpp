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
                             0x9A, 0x95, 0xA3, 0xA5,
                             0xA6, 0xA9, 0xAA, 0xAC, 
                             0xB1, 0xB2, 0xB4, 0xC3, 
                             0xC5, 0xC6, 0xC9, 0xCA, 
                             0xCC, 0xD1, 0xD2, 0xD4};

uint32_t Rd53aCmd::encode5to8(uint32_t val) {
    return Rd53aCmd::enc5to8[val & 0x1F];
}

void Rd53aCmd::trigger(uint32_t bc, uint32_t tag) {

}

void Rd53aCmd::ecr() {

}

void Rd53aCmd::bcr() {

}

void Rd53aCmd::globalPulse(uint32_t chipId, uint32_t duration) {

}

void Rd53aCmd::cal(uint32_t chipId, uint32_t mode, uint32_t delay, uint32_t duration, uint32_t aux_mode, uint32_t aux_delay) {

}

void Rd53aCmd::wrRegister(uint32_t chipId, uint32_t address, uint32_t value) {
std::ofstream commandFile;
commandFile.open("commandList.txt", std::ofstream::out | std::ofstream::app);
    if (verbose) std::cout << __PRETTY_FUNCTION__ << " : ID(" << chipId << ") ADR(" << address << ") VAL(0x" << std::hex << value << std::dec << ")" << std::endl;
    // Header
    core->writeFifo(0x6666);
commandFile << std::hex << 0x6666;
    uint32_t tmp = 0x0;
    // ID[3:0],0 | ADR[8:4]
    tmp += (this->encode5to8((chipId & 0xF) << 1)) << 24;
    tmp += (this->encode5to8((address >> 4) & 0x1F)) << 16;
    // ADR[3:0],VAL[15] | VAL[14:10]
    tmp += (this->encode5to8(((address & 0xF) << 1) + ((value >> 15) & 0x1)) << 8);
    tmp += (this->encode5to8((value >> 10) & 0x1F));
    core->writeFifo(tmp);
commandFile << std::hex << tmp;
    // VAL[9:5] | VAL [4:0]
    tmp = (this->encode5to8((value >> 5) & 0x1F) << 24);
    tmp += (this->encode5to8(value & 0x1F)) << 16;
    core->writeFifo(tmp);
commandFile << std::hex << tmp;
    core->releaseFifo();
commandFile << "\n";
commandFile.close();
}

void Rd53aCmd::wrRegister(uint32_t chipId, uint32_t address, uint32_t values[3]) {

}

void Rd53aCmd::rdRegister(uint32_t chipId, uint32_t address) {

}

