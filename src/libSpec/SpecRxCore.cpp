#include "SpecRxCore.h"
#include <cstring>

SpecRxCore::SpecRxCore() {
    verbose = false;
}

void SpecRxCore::setRxEnable(uint32_t value) {
    if (verbose)
        std::cout << __PRETTY_FUNCTION__ << " : Value 0x" << std::hex << value << std::dec << std::endl;
    SpecCom::writeSingle(RX_ADDR | RX_ENABLE, value);
}

void SpecRxCore::maskRxEnable(uint32_t value, uint32_t mask) {
    uint32_t tmp = SpecCom::readSingle(RX_ADDR | RX_ENABLE);
    tmp &= ~mask;
    value |= tmp;
    if (verbose)
        std::cout << __PRETTY_FUNCTION__ << " : Value 0x" << std::hex << value << std::dec << std::endl;
    SpecCom::writeSingle(RX_ADDR | RX_ENABLE, value);
}

RawData* SpecRxCore::readData() {
    uint32_t dma_addr = getStartAddr();
    uint32_t dma_count = getDataCount();
    uint32_t real_dma_count = dma_count;
    if (dma_count > 0 && dma_count < (251*256)) {
        real_dma_count = dma_count;
        if (dma_count%32 != 0)
            dma_count += 32-(dma_count%32);
        if (verbose)
            std::cout << __PRETTY_FUNCTION__ << " : Addr 0x" << std::hex <<
                dma_addr << " ,Count " << std::dec << dma_count << std::endl;
        uint32_t *buf = new uint32_t[dma_count];
        std::memset(buf, 0x0, sizeof(uint32_t)*dma_count);
        if (SpecCom::readDma(dma_addr, buf, dma_count)) {
            std::cout << __PRETTY_FUNCTION__ << std::hex << "0x" << dma_addr << " 0x" << dma_count << std::dec << std::endl;
            exit(1);
            return NULL;
        }
        return new RawData(dma_addr, buf, real_dma_count);
    } else {
        return NULL;
    }
}

void SpecRxCore::flushBuffer() {
    if (verbose)
        std::cout << __PRETTY_FUNCTION__ << std::endl;
    SpecCom::flushDma();
}

uint32_t SpecRxCore::getDataRate() {
    return SpecCom::readSingle(RX_BRIDGE | RX_DATA_RATE);
}

uint32_t SpecRxCore::getStartAddr() {
    return SpecCom::readSingle(RX_BRIDGE | RX_START_ADDR);
}

uint32_t SpecRxCore::getDataCount() {
    return SpecCom::readSingle(RX_BRIDGE | RX_DATA_COUNT);
}

bool SpecRxCore::isBridgeEmpty() {
    return SpecCom::readSingle(RX_BRIDGE | RX_BRIDGE_EMPTY);
}

uint32_t SpecRxCore::getCurCount() {
    return SpecCom::readSingle(RX_BRIDGE | RX_CUR_COUNT);
}
