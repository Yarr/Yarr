#ifndef BDAQTXCORE_H
#define BDAQTXCORE_H

#include <iostream>
#include <vector>
#include "TxCore.h"
#include "Bdaq53.h"

class BdaqTxCore : virtual public TxCore, virtual public Bdaq53 {
    public:
        BdaqTxCore();
        ~BdaqTxCore();

        void init();
        
        // Write to FE interface
        void writeFifo(uint32_t);
        void releaseFifo() {} // Not used. Commands are released with isCmdEmpty().
        void setCmdEnable(uint32_t); 
        void setCmdEnable(std::vector<uint32_t>);
        void disableCmd() {} // Future implementation.
        uint32_t getCmdEnable(); 
        bool isCmdEmpty();

        // Word repeater TODO: move to seperate class?
        void setTrigEnable(uint32_t value); 
        uint32_t getTrigEnable();
        void maskTrigEnable(uint32_t value, uint32_t mask);
        bool isTrigDone();
        void setTrigConfig(enum TRIG_CONF_VALUE cfg); 
        void setTrigFreq(double freq); // in Hz
        void setTrigCnt(uint32_t count);
        void setTrigTime(double time); // in s
        void setTrigWordLength(uint32_t length); // From Msb
        void setTrigWord(uint32_t *word, uint32_t length); // 4 words, start at Msb
        void toggleTrigAbort();

        // Trigger interface (This is the TLU stuff)
        void setTriggerLogicMask(uint32_t mask) {}
        void setTriggerLogicMode(enum TRIG_LOGIC_MODE_VALUE mode) {}
        void resetTriggerLogic() {}
        uint32_t getTrigInCount() {}
    protected:

    private:
        bool verbose;
        uint16_t trgRepetitions = 0;
        uint32_t trgEnable = 0;
        std::vector<uint8_t> cmdData;
        std::vector<uint8_t> trgData; //Let's keep things separated for now.
        void sendCommand();
};

#endif
