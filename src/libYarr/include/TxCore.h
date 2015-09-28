#ifndef TXCORE_H
#define TXCORE_H

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

#include "SpecController.h"

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
#define TRIG_ABORT 0xF

#define TX_CLK_PERIOD 25e-9

enum TRIG_CONF_VALUE {
    EXT_TRIGGER = 0x0,
    INT_TIME = 0x1,
    INT_COUNT = 0x2
};

class TxCore {
    public:
        TxCore(SpecController *arg_spec);

        void setVerbose(bool v=true);

        void writeFifo(uint32_t value);
        
        void setCmdEnable(uint32_t value);
        uint32_t getCmdEnable();
        void maskCmdEnable(uint32_t value, uint32_t mask);

        void setTrigEnable(uint32_t value);
        uint32_t getTrigEnable();
        void maskTrigEnable(uint32_t value, uint32_t mask);

        void setTrigConfig(enum TRIG_CONF_VALUE cfg);
        void setTrigFreq(double freq); // in Hz
        void setTrigCnt(uint32_t count);
        void setTrigTime(double time); // in s
        void setTrigWordLength(uint32_t length); // From Msb
        void setTrigWord(uint32_t *word); // 4 words, start at Msb

        void toggleTrigAbort();

        bool isCmdEmpty();
        bool isTrigDone();
    private:
        SpecController *spec;
        bool verbose;
        uint32_t enMask;
};

#endif
