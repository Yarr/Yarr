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
#include "Rd53a.h"
#include "Rd53aEmu.h"

template<>
void EmuTxCore<Fei4>::doTrigger() {
    for(unsigned i=0; i<m_trigCnt; i++) {
        m_com->write32(0x1D000000 + i);
    }
    m_com->write32(0x0);
    while(!m_com->isEmpty());
    trigProcRunning = false;
}


template<>
void EmuTxCore<Rd53a>::doTrigger() {
    for(unsigned i=0; i<m_trigCnt; i++) {
        for( uint32_t j =0; j<trigLength; j++) {
            m_com->write32( trigWord[trigLength-j-1] );
        }
    }
    m_com->write32(0x0);
    while(!m_com->isEmpty());
    trigProcRunning = false;
    //std::cout << __PRETTY_FUNCTION__ << ": doTrigger() is done." << std::endl;
}


