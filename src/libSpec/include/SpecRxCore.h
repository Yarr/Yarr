#ifndef SPECRXCORE_H
#define SPECRXCORE_H

// #################################
// # Author: Timon Heim
// # Email: timon.heim at cern.ch
// # Project: Yarr
// # Description: YARR FW Library
// # Comment: Receiver Core
// ################################

#include <iostream>

#include "SpecCom.h"
#include "RxCore.h"
#include "RawData.h"

#define RX_ADDR (0x2 << 14)
#define RX_BRIDGE (0x3 << 14)

#define RX_ENABLE 0x0
#define RX_STATUS 0x1

#define RX_START_ADDR 0x0
#define RX_DATA_COUNT 0x1
#define RX_LOOPBACK 0x2
#define RX_DATA_RATE 0x3
#define RX_LOOP_FIFO 0x4
#define RX_BRIDGE_EMPTY 0x5
#define RX_CUR_COUNT 0x6

class SpecRxCore : virtual public RxCore, virtual public SpecCom{
    public:
        SpecRxCore();

        void setRxEnable(uint32_t val);
        void maskRxEnable(uint32_t val, uint32_t mask);

        RawData* readData();
        void flushBuffer();
        
        uint32_t getDataRate();
        uint32_t getCurCount();
        bool isBridgeEmpty();

    private:
        bool verbose;

        uint32_t getStartAddr();
        uint32_t getDataCount();
};

#endif
