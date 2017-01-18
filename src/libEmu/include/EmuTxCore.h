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

#include "TxCore.h"

class EmuTxCore : public TxCore {
    public:
        EmuTxCore();
        ~EmuTxCore();

        void writeFifo(uint32_t value){}
        
        void setCmdEnable(uint32_t value) {}
        uint32_t getCmdEnable() {return 0x0;}
        void maskCmdEnable(uint32_t value, uint32_t mask) {}

        void setTrigEnable(uint32_t value) {}
        uint32_t getTrigEnable() {return 0x0;}
        void maskTrigEnable(uint32_t value, uint32_t mask) {}

        void setTrigConfig(enum TRIG_CONF_VALUE cfg) {}
        void setTrigFreq(double freq) {}
        void setTrigCnt(uint32_t count) {}
        void setTrigTime(double time) {}
        void setTrigWordLength(uint32_t length) {}
        void setTrigWord(uint32_t *word) {}

        void toggleTrigAbort() {}

        bool isCmdEmpty() {return true;}
        bool isTrigDone() {return true;}

        uint32_t getTrigInCount() {return 0x0;}
        
        void setTriggerLogicMask(uint32_t mask) {}
        void setTriggerLogicMode(enum TRIG_LOGIC_MODE_VALUE mode) {}
        void resetTriggerLogic() {}

    private:

};

#endif
