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

/**
 * Core implementation of DAQ data receiver.
 */
class RxCore {
    public:
        /** Set enabled rx channels */
        virtual void setRxEnable(uint32_t val) = 0;
        virtual void setRxEnable(std::vector<uint32_t>) = 0;
        /** Change enabled rx channels */
        virtual void maskRxEnable(uint32_t val, uint32_t mask) = 0;
        virtual void disableRx() = 0;
        virtual void checkRxSync() {}
        virtual void initRxChannels(const std::vector<uint32_t>& channels) {}

        /**
         * Read raw data.
         */
        virtual std::vector<RawDataPtr > readData() = 0;

        virtual void flushBuffer() {}
        
        virtual uint32_t getDataRate() = 0;
        /// How much data is available to be be read.
        virtual uint32_t getCurCount() {return 0;};
        virtual bool isBridgeEmpty() = 0;
        std::chrono::microseconds getWaitTime() {
            return m_waitTime;
        }

        // Controller firmware register access
        // return true if operation is successful, otherwise false
        virtual bool readFwRegister(const std::string& name, uint64_t& value) { return false; }
        virtual bool writeFwRegister(const std::string& name, const uint64_t& value) { return false; }

    protected:
        RxCore()=default;
        virtual ~RxCore()=default;

        std::chrono::microseconds m_waitTime{100}; // typical latency in the HW controller RX path
};

#endif
