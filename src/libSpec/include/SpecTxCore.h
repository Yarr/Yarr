#ifndef SPECTXCORE_H
#define SPECTXCORE_H

// #################################
// # Author: Timon Heim
// # Email: timon.heim at cern.ch
// # Project: Yarr
// # Description: YARR FW Library
// # Comment: Transmitter Core
// ################################

#include <iostream>
#include <stdint.h>
#include <thread>
#include <chrono>
#include <vector>

#include "SpecCom.h"
#include "TxCore.h"

#define TX_ADDR (0x1 << 14)

#define TX_FIFO 0x0
#define TX_ENABLE 0x1
#define TX_EMPTY 0x2
#define TRIG_EN 0x3
#define TRIG_DONE 0x4
#define TRIG_CONF 0x5
#define TRIG_FREQ 0x6
#define TRIG_TIME 0x7
#define TRIG_COUNT 0x9
#define TRIG_WORD_LENGTH 0xA
#define TRIG_WORD 0xB
#define TRIG_WORD_POINTER 0xC
#define TX_PULSE_WORD 0xD
#define TX_PULSE_INTERVAL 0xE
#define TRIG_ABORT 0xF
#define TRIG_IN_CNT 0xF
#define TX_POLARITY 0x10
#define TX_SYNC_WORD 0x11
#define TX_SYNC_INTERVAL 0x12
#define TX_IDLE_WORD 0x13

#define TX_CLK_PERIOD 25e-9

// TODO move into its own class
#define TRIG_LOGIC_ADR (0x5 << 14)
#define TRIG_LOGIC_MASK 0x0
#define TRIG_LOGIC_MODE 0x1
#define TRIG_LOGIC_CONFIG 0x2
#define TRIG_LOGIC_EDGE 0x3
#define TRIG_LOGIC_DELAY 0x4 // And the next 3 addresses up to 0x7
#define TRIG_LOGIC_DEADTIME 0x8

#define NCHANNELS 4


class SpecTxCore : virtual public TxCore, virtual public SpecCom{
    public:
        SpecTxCore();

        void writeFifo(uint32_t value) override;
        void releaseFifo() override {};
        
        void setCmdEnable(uint32_t value) override;
        void setCmdEnable(std::vector<uint32_t> channels) override;
        void disableCmd() override;
        uint32_t getCmdEnable() override;
        void maskCmdEnable(uint32_t value, uint32_t mask);

        void setTrigEnable(uint32_t value) override;
        uint32_t getTrigEnable() override;
        void maskTrigEnable(uint32_t value, uint32_t mask) override;

        void setTrigConfig(enum TRIG_CONF_VALUE cfg) override;
        void setTrigFreq(double freq) override; // in Hz
        void setTrigCnt(uint32_t count) override;
        void setTrigTime(double time) override; // in s
        void setTrigWordLength(uint32_t length) override; // From Msb
        void setTrigWord(uint32_t *word, uint32_t length) override; // 4 words, start at Msb

        void toggleTrigAbort() override;

        bool isCmdEmpty() override;
        bool isTrigDone() override;

        uint32_t getTrigInCount() override;
        
        // TODO move to own class
        void setTriggerLogicMask(uint32_t mask) override {
            SpecCom::writeSingle(TRIG_LOGIC_ADR | TRIG_LOGIC_MASK, mask);
        };
        void setTriggerLogicMode(enum TRIG_LOGIC_MODE_VALUE mode) override {
            SpecCom::writeSingle(TRIG_LOGIC_ADR | TRIG_LOGIC_MODE, (uint32_t) mode);
        }
        void setTriggerLogicConfig(uint32_t config) {
            SpecCom::writeSingle(TRIG_LOGIC_ADR | TRIG_LOGIC_CONFIG, config);
        }
        void setTriggerEdge(uint32_t edge) {
            SpecCom::writeSingle(TRIG_LOGIC_ADR | TRIG_LOGIC_EDGE, edge);
        }
        void setTriggerDelay(uint32_t channel, uint32_t delay) {
            if (channel < NCHANNELS) 
                SpecCom::writeSingle((TRIG_LOGIC_ADR | TRIG_LOGIC_DELAY) + channel, delay);
        }
        void setTriggerDeadtime(uint32_t deadtime) {
            SpecCom::writeSingle(TRIG_LOGIC_ADR | TRIG_LOGIC_DEADTIME, deadtime);
        }

        void setPulseWord(uint32_t word) {
            SpecCom::writeSingle(TX_ADDR | TX_PULSE_WORD, word);
        }

        void setPulseInterval(uint32_t interval) {
            SpecCom::writeSingle(TX_ADDR | TX_PULSE_INTERVAL, interval & 0xFFFF);
        }

        void setSyncWord(uint32_t word) {
            SpecCom::writeSingle(TX_ADDR | TX_SYNC_WORD, word);
        }

        void setSyncInterval(uint32_t interval) {
            SpecCom::writeSingle(TX_ADDR | TX_SYNC_INTERVAL, interval & 0xFF);
        }
        
        void setIdleWord(uint32_t word) {
            SpecCom::writeSingle(TX_ADDR | TX_IDLE_WORD, word);
        }
        
        void resetTriggerLogic() override {
            SpecCom::writeSingle(TRIG_LOGIC_ADR | 0xFF, 0x1);
        }

        void setTxPolarity(uint32_t value);
        uint32_t getTxPolarity();
    protected:
        double m_clk_period;

    private:
        //uint32_t enMask;
};

#endif
