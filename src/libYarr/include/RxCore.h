#ifndef RXCORE_H
#define RXCORE_H

// #################################
// # Author: Timon Heim
// # Email: timon.heim at cern.ch
// # Project: Yarr
// # Description: Abstract receiver
// # Comment:
// # Data: Dec 2016
// ################################

#include <cstdint>

#include "RawData.h"

class RxCore {
    public:
        virtual void setRxEnable(uint32_t val) = 0;
        virtual void maskRxEnable(uint32_t val, uint32_t mask) = 0;

        virtual RawData* readData() = 0;
        virtual void flushBuffer() {}
        
        virtual uint32_t getDataRate() = 0;
        virtual uint32_t getCurCount() = 0;
        virtual bool isBridgeEmpty() = 0;

    protected:
        RxCore();
        ~RxCore();
};

#endif
