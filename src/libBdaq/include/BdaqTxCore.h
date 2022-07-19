#ifndef BDAQTXCORE_H
#define BDAQTXCORE_H

#include <iostream>
#include <vector>
#include <thread>
#include "TxCore.h"
#include "Bdaq.h"

class BdaqTxCore : virtual public TxCore, virtual public Bdaq {
    public:
        BdaqTxCore();
        ~BdaqTxCore() override;

        void init();
        
        // Write to FE interface
        void writeFifo(uint32_t) override;
        void releaseFifo() override {} // Not used. Commands are released with isCmdEmpty().
        void setCmdEnable(uint32_t) override;
        void setCmdEnable(std::vector<uint32_t>) override;
        void disableCmd() override {} // Future implementation.
        uint32_t getCmdEnable() override;
        bool isCmdEmpty() override;

        // Word repeater TODO: move to seperate class?
        void setTrigEnable(uint32_t value) override;
        uint32_t getTrigEnable() override;
        void maskTrigEnable(uint32_t value, uint32_t mask) override;
        bool isTrigDone() override;
        void setTrigConfig(enum TRIG_CONF_VALUE cfg) override;
        void setTrigFreq(double freq) override; // in Hz
        void setTrigCnt(uint32_t count) override;
        void setTrigTime(double time) override; // in s
        void setTrigWordLength(uint32_t length) override; // From Msb
        void setTrigWord(uint32_t *word, uint32_t length) override; // 4 words, start at Msb
        void toggleTrigAbort() override;
        bool getSoftwareAZ() override { return m_softwareAZ; }

        // Trigger interface (This is the TLU stuff)
        void setTriggerLogicMask(uint32_t mask) override {}
        void setTriggerLogicMode(enum TRIG_LOGIC_MODE_VALUE mode) override {}
        void resetTriggerLogic() override {}
        uint32_t getTrigInCount() override { return 0; }
    protected:
        bool m_softwareAZ;

    private:
        // Registers Configuration
        std::vector<uint8_t> cmdData;
        std::vector<uint8_t> cmdData1;
        std::vector<uint8_t> cmdData2;
        void sendCommand();

        // Common Command Repeater (Trigger)
        uint32_t trgEnable = 0; // Emulating SPEC register.
        std::vector<uint8_t> trgData;
        
        // Hardware Command Repeater (Trigger)
        uint16_t hardwareTriggerCount = 0;
        uint hardwareTriggerNoop = 0;
        void hardwareTriggerSet();
        void hardwareTriggerRun();

        // Emulated Timed Trigger 
        bool timedTrigger = false;
        bool timedTriggerAbort = false;
        bool timedTriggerDone = false;
        double timedTriggerFreq = 0;
        double timedTriggerTime = 0;
        std::thread timedTriggerThread;
        void timedTriggerSet();
        void timedTriggerRun();

};

#endif
