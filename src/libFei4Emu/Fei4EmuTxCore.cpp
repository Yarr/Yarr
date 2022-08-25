// #################################
// # Author: Timon Heim
// # Email: timon.heim at cern.ch
// # Project: Yarr
// # Description: Emulator Transmitter
// # Comment:
// # Date: Jan 2017
// ################################

#include "EmuTxCore.h"
#include "Fei4.h"

template<>
void EmuTxCore<Fei4>::doTrigger() {
    for(unsigned i=0; i<m_trigCnt; i++) {
        for (auto& ch : m_channels) {
            if (not ch.second) continue;
            m_coms[ch.first]->write32(0x1D000000 + i);
        }
    }
    EmuTxCore<Fei4>::writeFifo(0x0);
    while(not EmuTxCore<Fei4>::isCmdEmpty());
    trigProcRunning = false;
}
