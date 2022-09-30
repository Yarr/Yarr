#ifndef EMUCOM_H
#define EMUCOM_H

#include <cstdint>

class EmuCom {
    public:
        /// Get current FIFO occupancy (in bytes)
        virtual uint32_t getCurSize() = 0;
        /// Nothing in buffer
        virtual bool isEmpty() = 0;
        /// Read single element
        virtual uint32_t read32() = 0;
        /// Read multiple elements
        virtual uint32_t readBlock32(uint32_t *buf, uint32_t length) = 0;
        /// Write element to queueb
        virtual void write32(uint32_t) = 0;

        virtual ~EmuCom();
    protected:
        EmuCom();
};

#endif
