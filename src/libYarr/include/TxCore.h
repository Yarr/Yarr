#ifndef TXCORE_H
#define TXCORE_H

// #################################
// # Author: Timon Heim
// # Email: timon.heim at cern.ch
// # Project: Yarr
// # Description: YARR Hardware Abstraction Layer
// # Comment: Transmitter Core
// ################################

#include <cstdint>

enum TRIG_CONF_VALUE {
    EXT_TRIGGER = 0x0,
    INT_TIME = 0x1,
    INT_COUNT = 0x2
};

enum TRIG_LOGIC_MODE_VALUE { 
    MODE_L1A_COUNT = 0x0,
    MODE_TIMESTAMP = 0x1,
    MODE_EUDET_TAG = 0x2
};

class TxCore {
    public:
        // Write to FE interface
        virtual void writeFifo(uint32_t) = 0;
        virtual void releaseFifo() = 0;
        virtual void setCmdEnable(uint32_t) = 0;
        virtual uint32_t getCmdEnable() = 0;
        virtual bool isCmdEmpty() = 0;

        // Word repeater TODO: move to seperate class?
        virtual void setTrigEnable(uint32_t value) = 0;
        virtual uint32_t getTrigEnable() = 0;
        virtual void maskTrigEnable(uint32_t value, uint32_t mask) = 0;
        virtual bool isTrigDone() = 0;


        virtual void setTrigConfig(enum TRIG_CONF_VALUE cfg) = 0;
        virtual void setTrigFreq(double freq) = 0; // in Hz
        virtual void setTrigCnt(uint32_t count) = 0;
        virtual void setTrigTime(double time) = 0; // in s
        virtual void setTrigWordLength(uint32_t length) = 0; // From Msb
        virtual void setTrigWord(uint32_t *word, uint32_t length) = 0; // 4 words, start at Msb
        virtual void toggleTrigAbort() = 0;

        // Trigger interface
        virtual void setTriggerLogicMask(uint32_t mask) = 0;
        virtual void setTriggerLogicMode(enum TRIG_LOGIC_MODE_VALUE mode) = 0;
        virtual void resetTriggerLogic() = 0;
        virtual uint32_t getTrigInCount() = 0;

        void setClkPeriod(double period) {
            m_clk_period = period;
        }
    protected:
        TxCore();
        ~TxCore();
        uint32_t enMask;
        double m_clk_period;
};

#endif
