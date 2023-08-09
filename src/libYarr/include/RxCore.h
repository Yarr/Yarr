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
#include <vector>
#include <chrono>

#include "RawData.h"

class RxCore {
    public:
        virtual void setRxEnable(uint32_t val) = 0;
        virtual void setRxEnable(std::vector<uint32_t>) = 0;
        virtual void maskRxEnable(uint32_t val, uint32_t mask) = 0;
        virtual void disableRx() = 0;
        virtual void checkRxSync() {}

        virtual std::vector<RawDataPtr > readData() = 0;
        virtual void flushBuffer() {}
        
        virtual uint32_t getDataRate() = 0;
        virtual uint32_t getCurCount() {return 0;};
        virtual bool isBridgeEmpty() = 0;
        std::chrono::microseconds getWaitTime() {
            return m_waitTime;
        }
        uint32_t getTriggersLostTolerance() {
          return m_triggersLostTolerance;
        }
        std::chrono::microseconds getAverageDataProcessingTime() {
          return m_averageDataProcessingTime;
        }

    protected:
        RxCore()=default;
        virtual ~RxCore()=default;

        std::chrono::microseconds m_waitTime{100}; // typical latency in the HW controller RX path
        uint32_t m_triggersLostTolerance = 0; // allowed number of lost triggers
        std::chrono::microseconds m_averageDataProcessingTime{100};

};

#endif
