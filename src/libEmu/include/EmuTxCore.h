#ifndef EMUTXCORE_H
#define EMUTXCORE_H

// #################################
// # Author: Timon Heim
// # Email: timon.heim at cern.ch
// # Project: Yarr
// # Description: Emulator Transmitter 
// # Comment: 
// # Date: Jan 2017
// ################################

#include <atomic>
#include <iostream>
#include <thread>
#include <mutex>
#include <map>

#include "TxCore.h"
#include "EmuCom.h"

template<class FE>
class EmuTxCore : virtual public TxCore {
    public:
        EmuTxCore();
        ~EmuTxCore();

        void setCom(uint32_t chn, EmuCom *com);
        EmuCom* getCom(uint32_t chn);

        void writeFifo(uint32_t value);
        void writeFifo(uint32_t chn, uint32_t value);
        void releaseFifo() {this->writeFifo(0x0);} // Add some padding

        // TODO
        void setCmdEnable(uint32_t value) {}
        void setCmdEnable(std::vector<uint32_t> channels) {}
        void disableCmd() {}
        uint32_t getCmdEnable() {return 0x0;}
        void maskCmdEnable(uint32_t value, uint32_t mask) {}

        void setTrigEnable(uint32_t value);
        uint32_t getTrigEnable() {return triggerProc.joinable() || !EmuTxCore<FE>::isCmdEmpty();}
        void maskTrigEnable(uint32_t value, uint32_t mask) {}

        void setTrigConfig(enum TRIG_CONF_VALUE cfg) {}
        void setTrigFreq(double freq) {}
        void setTrigCnt(uint32_t count);
        void setTrigTime(double time) {}
        
        void setTrigWordLength(uint32_t length) { trigLength = length; }
        void setTrigWord(uint32_t *word, uint32_t length) { trigWord = word; trigLength = length; }

        void toggleTrigAbort() {}

        bool isCmdEmpty() {
            for (auto& com : m_coms) {
                if (m_channels[com.first])
                    if (not com.second->isEmpty()) return false;
            }
            return true;
        }

        bool isTrigDone() {
            bool rtn = !trigProcRunning && EmuTxCore<FE>::isCmdEmpty();
            return rtn;
        }

        uint32_t getTrigInCount() {return 0x0;}
        
        void setTriggerLogicMask(uint32_t mask) {}
        void setTriggerLogicMode(enum TRIG_LOGIC_MODE_VALUE mode) {}
        void resetTriggerLogic() {}

    private:
        std::map<uint32_t, EmuCom*> m_coms;
        std::map<uint32_t, bool> m_channels;

        unsigned m_trigCnt;
        std::mutex accMutex;
        std::thread triggerProc;
        std::atomic<bool> trigProcRunning;
    uint32_t* trigWord;
    uint32_t trigLength;
        void doTrigger();
};

template<class FE>
EmuTxCore<FE>::EmuTxCore() {
    m_trigCnt = 0;
    trigProcRunning = false;
}

template<class FE>
EmuTxCore<FE>::~EmuTxCore() {}

template<class FE>
void EmuTxCore<FE>::setCom(uint32_t chn, EmuCom *com) {
    m_coms[chn] = com;
    m_channels[chn] = true;
}

template<class FE>
EmuCom* EmuTxCore<FE>::getCom(uint32_t chn) {
    if (m_coms.find(chn) != m_coms.end()) {
        return m_coms[chn];
    } else {
        return nullptr;
    }
}

template<class FE>
void EmuTxCore<FE>::writeFifo(uint32_t value) {
    for (auto& chn_en : m_channels) {
        if (chn_en.second) {
            EmuTxCore<FE>::writeFifo(chn_en.first, value);
        }
    }
}

template<class FE>
void EmuTxCore<FE>::writeFifo(uint32_t chn, uint32_t value) {
    if (m_channels[chn]) m_coms[chn]->write32(value);
}

template<class FE>
void EmuTxCore<FE>::setTrigCnt(uint32_t count) {
    m_trigCnt = count;
}

template<class FE>
void EmuTxCore<FE>::setTrigEnable(uint32_t value) {
    // TODO value should reflect channel
    if(value == 0) {
        if (triggerProc.joinable()) triggerProc.join();
    } else {
        trigProcRunning = true;
        triggerProc = std::thread(&EmuTxCore::doTrigger, this);
    }
}

#endif
