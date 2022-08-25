// #################################
// # Author: Timon Heim
// # Email: timon.heim at cern.ch
// # Project: Yarr
// # Description: Emulator Transmitter
// # Comment:
// # Date: Jan 2017
// ################################

#include "EmuTxCore.h"
#include "Rd53a.h"
// #include "Rd53aEmu.h"

template<>
void EmuTxCore<Rd53a>::doTrigger() {
    for(unsigned i=0; i<m_trigCnt; i++) {
        for (auto& ch : m_channels) {
            if (not ch.second) continue;
            for( uint32_t j =0; j<trigLength; j++) {
                m_coms[ch.first]->write32( trigWord[trigLength-j-1] );
            }
        }
    }
    EmuTxCore<Rd53a>::writeFifo(0x0);
    while(not EmuTxCore<Rd53a>::isCmdEmpty());
    trigProcRunning = false;
    //std::cout << __PRETTY_FUNCTION__ << ": doTrigger() is done." << std::endl;
}
