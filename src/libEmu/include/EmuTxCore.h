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

#include <iostream>
#include <thread>
#include <mutex>

#include "TxCore.h"
#include "EmuCom.h"

template<class FE>
class EmuTxCore : virtual public TxCore {
    public:
        EmuTxCore(EmuCom *com);
        EmuTxCore();
        ~EmuTxCore();

        void setCom(EmuCom *com) {m_com = com;}
        EmuCom* getCom() {return m_com;}

        void writeFifo(uint32_t value);
        void releaseFifo() {this->writeFifo(0x0);} // Add some padding
        
        void setCmdEnable(uint32_t value) {}
        uint32_t getCmdEnable() {return 0x0;}
        void maskCmdEnable(uint32_t value, uint32_t mask) {}

        void setTrigEnable(uint32_t value);
        uint32_t getTrigEnable() {return triggerProc.joinable() || !m_com->isEmpty();}
        void maskTrigEnable(uint32_t value, uint32_t mask) {}

        void setTrigConfig(enum TRIG_CONF_VALUE cfg) {}
        void setTrigFreq(double freq) {}
        void setTrigCnt(uint32_t count);
        void setTrigTime(double time) {}
    void setTrigWordLength(uint32_t length) { trigLength = length; }
    void setTrigWord(uint32_t *word, uint32_t length) { trigWord = word; trigLength = length; }

        void toggleTrigAbort() {}

        bool isCmdEmpty() {
            bool rtn = m_com->isEmpty();
            return rtn;
        }
        bool isTrigDone() {
            bool rtn = !trigProcRunning && m_com->isEmpty();
            return rtn;
        }

        uint32_t getTrigInCount() {return 0x0;}
        
        void setTriggerLogicMask(uint32_t mask) {}
        void setTriggerLogicMode(enum TRIG_LOGIC_MODE_VALUE mode) {}
        void resetTriggerLogic() {}

    private:
        EmuCom *m_com;

        unsigned m_trigCnt;
        std::mutex accMutex;
        std::thread triggerProc;
        bool trigProcRunning;
    uint32_t* trigWord;
    uint32_t trigLength;
        void doTrigger();
};

template<class FE>
EmuTxCore<FE>::EmuTxCore(EmuCom *com) {
    m_com = com;
    m_trigCnt = 0;
    trigProcRunning = false;
}

template<class FE>
EmuTxCore<FE>::EmuTxCore() {
    m_com = NULL;
    m_trigCnt = 0;
    trigProcRunning = false;
}

template<class FE>
EmuTxCore<FE>::~EmuTxCore() {}

template<class FE>
void EmuTxCore<FE>::writeFifo(uint32_t value) {
    // TODO need to check channel
    m_com->write32(value);
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
