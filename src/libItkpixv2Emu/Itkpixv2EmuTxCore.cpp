/*
* Author: Ondra Kovanda, ondrej.kovanda at cern.ch
* Date: 05/2024
* Description: Core functionality of the ITkPixV2 emulator
*/

// adapted from:
// #################################
// # Author: Timon Heim
// # Email: timon.heim at cern.ch
// # Project: Yarr
// # Description: Emulator Transmitter
// # Comment:
// # Date: Jan 2017
// ################################

#include "EmuTxCore.h"
#include "Itkpixv2.h"

//implement the FE-specific triggering through the virtual Tx lane
template<>
void EmuTxCore<Itkpixv2>::doTrigger() {
    for(unsigned i=0; i<m_trigCnt; i++) {
        for (auto& ch : m_channels) {
            if (not ch.second) continue;
            for( uint32_t j =0; j<trigLength; j++) {
                m_coms[ch.first]->write32( trigWord[trigLength-j-1] );
            }
        }
    }
    EmuTxCore<Itkpixv2>::writeFifo(0x0);
    while(not EmuTxCore<Itkpixv2>::isCmdEmpty());
    trigProcRunning = false;
}
