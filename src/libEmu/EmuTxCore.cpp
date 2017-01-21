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
    m_trigCnt = 0;
    trigProcRunning = false;
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
    // TODO value should reflect channel
    if(value == 0) {
        triggerProc.join();
    } else {
        trigProcRunning = true;
        triggerProc = std::thread(&EmuTxCore::doTrigger, this);
    }
}

void EmuTxCore::doTrigger() {
    for(unsigned i=0; i<m_trigCnt; i++) {
        m_com->write32(0x1D000000 + i);
    }
    while(!m_com->isEmpty());
    trigProcRunning = false;
}
