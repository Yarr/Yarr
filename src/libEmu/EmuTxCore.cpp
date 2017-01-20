// #################################
// # Author: Timon Heim
// # Email: timon.heim at cern.ch
// # Project: Yarr
// # Description: Emulator Transmitter
// # Comment:
// # Date: Jan 2017
// ################################

#include "EmuTxCore.h"

EmuTxCore::EmuTxCore(EmuCom *com) {
    m_com = com;
}

EmuTxCore::~EmuTxCore() {}

void EmuTxCore::writeFifo(uint32_t value) {
    // TODO need to check channel
    m_com->write32(value);
}

void EmuTxCore::setTrigCnt(uint32_t count) {
    m_trigCnt = count;
}

void EmuTxCore::setTrigEnable(uint32_t value) {
    if(!value) return;
    std::cout << __PRETTY_FUNCTION__ << std::endl;
    m_com->write32(0x1D000000 | (m_trigCnt&0xFFFFFF));
    m_com->write32(0xDEADBEEF);
}
