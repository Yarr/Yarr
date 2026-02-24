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
#include <vector>
#include <string>

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

/**
 * Core DAQ interface, tx side.
 *
 * Represents one DAQ card with some number of channels.
 */
class TxCore {
    public:
        /// Write word to internal FIFO
        virtual void writeFifo(uint32_t) = 0;
        /// Send contents of FIFO to FE interface
        virtual void releaseFifo() = 0;
        /// Set channel enable mask for commands
        virtual void setCmdEnable(uint32_t) = 0;
        virtual void setCmdEnable(std::vector<uint32_t>) = 0;
        virtual void disableCmd() = 0;
        /// Get channel mask
        virtual uint32_t getCmdEnable() = 0;
        /// Is command sending complete
        virtual bool isCmdEmpty() = 0;

        /**
         * Repeat triggers (aka burster).
         */
        // Word repeater TODO: move to seperate class?
        virtual void setTrigEnable(uint32_t value) = 0;
        /// Are triggers being repeated
        virtual uint32_t getTrigEnable() = 0;
        /// Change trig enable value. Add bits in value, remove bits in mask
        virtual void maskTrigEnable(uint32_t value, uint32_t mask) = 0;
        /// Is trigger burst complete?
        virtual bool isTrigDone() = 0;

        /**
         * Return the maximum length of a trigger sequence.
         *
         * For example for spec card 32, for FELIX controller 16.
         * This can be overridden by controller implementation.
         */
        virtual int getMaxTrigWordLength() {return 32;}

        /// Configure how to end trigger burst.
        virtual void setTrigConfig(enum TRIG_CONF_VALUE cfg) = 0;
        /// Set trigger burster frequency
        virtual void setTrigFreq(double freq) = 0;
        /// How many triggers in a burst.
        virtual void setTrigCnt(uint32_t count) = 0;
        /// Set burst time.
        virtual void setTrigTime(double time) = 0; // in s
        /// How many bits? to send each time
        virtual void setTrigWordLength(uint32_t length) = 0; // From Msb
        /// Configure setting
        virtual void setTrigWord(uint32_t *word, uint32_t length) = 0; // 4 words, start at Msb
        /// Stop sending triggers
        virtual void toggleTrigAbort() = 0;

        // Software AZ (for select hw controllers)
        virtual bool getSoftwareAZ() { return false; }

        // Trigger interface
        virtual void setTriggerLogicMask(uint32_t mask) = 0;
        /// Set what to record about a trigger
        virtual void setTriggerLogicMode(enum TRIG_LOGIC_MODE_VALUE mode) = 0;
        virtual void resetTriggerLogic() = 0;
        /// Get the number of triggers in
        virtual uint32_t getTrigInCount() = 0;

        void setClkPeriod(double period) {
            m_clk_period = period;
        }

        // Controller firmware register access
        // return true if operation is successful, otherwise false
        virtual bool readFwRegister(const std::string& name, uint64_t& value) { return false; }
        virtual bool writeFwRegister(const std::string& name, const uint64_t& value) { return false; }

    protected:
        TxCore()=default;
        virtual ~TxCore()=default;
        uint32_t enMask {0};
        double m_clk_period {0.};
};

#endif
