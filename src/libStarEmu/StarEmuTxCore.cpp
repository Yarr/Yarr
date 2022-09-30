// #################################
// # Description: Emulator Transmitter for Star
// ################################

#include "EmuTxCore.h"
#include "StarChips.h"

template<>
void EmuTxCore<StarChips>::doTrigger() {
    for(unsigned i=0; i<m_trigCnt; i++) {
        for (auto& ch : m_channels) {
            if (not ch.second) continue;
            for( uint32_t j =0; j<trigLength; j++) {
                m_coms[ch.first]->write32( trigWord[trigLength-j-1] );
            }
        }
    }

    while(not EmuTxCore<StarChips>::isCmdEmpty());
    trigProcRunning = false;
    //std::cout << __PRETTY_FUNCTION__ << ": doTrigger() is done." << std::endl;
}
