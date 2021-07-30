#include "SpecRxCore.h"
#include <cstring>

#include "logging.h"

namespace {
    auto srxlog = logging::make_log("SpecRx");
}

SpecRxCore::SpecRxCore() {
    m_rxActiveLanes = 15; 
}

void SpecRxCore::setRxEnable(uint32_t value) {
    uint32_t mask = (1 << value);
    SPDLOG_LOGGER_TRACE(srxlog, "Value {0:x}", mask);
    SpecCom::writeSingle(RX_ADDR | RX_ENABLE, mask);
}

void SpecRxCore::setRxEnable(std::vector<uint32_t> channels) {
    uint32_t mask = 0;
    for (uint32_t channel : channels) {
        mask |= (1 << channel);

    }
    SPDLOG_LOGGER_TRACE(srxlog, "Value {0:x}", mask);
    SpecCom::writeSingle(RX_ADDR | RX_ENABLE, mask);

}

void SpecRxCore::disableRx() {
    SPDLOG_LOGGER_TRACE(srxlog, "");
    SpecCom::writeSingle(RX_ADDR | RX_ENABLE, 0x0);
}


void SpecRxCore::maskRxEnable(uint32_t value, uint32_t mask) {
    uint32_t tmp = SpecCom::readSingle(RX_ADDR | RX_ENABLE);
    tmp &= ~mask;
    value |= tmp;
    SPDLOG_LOGGER_TRACE(srxlog, "Value {0:x}", value);
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

        SPDLOG_LOGGER_DEBUG(srxlog, "Read data to Addr 0x{:x}, Count {}", dma_addr, dma_count);
        uint32_t *buf = new uint32_t[dma_count];
        std::memset(buf, 0x0, sizeof(uint32_t)*dma_count);
        if (SpecCom::readDma(dma_addr, buf, dma_count)) {
            SPDLOG_LOGGER_CRITICAL(srxlog, "Critical error while readin data ... aborting!!");
            exit(1);
        }
        return new RawData(dma_addr, buf, real_dma_count);
    } else {
        return NULL;
    }
}

void SpecRxCore::flushBuffer() {
    SPDLOG_LOGGER_TRACE(srxlog, "");
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

uint32_t SpecRxCore::getLinkStatus() {
    return SpecCom::readSingle(RX_ADDR | RX_STATUS);
}

void SpecRxCore::setRxPolarity(uint32_t value) {
    SpecCom::writeSingle(RX_ADDR | RX_POLARITY, value);
}

uint32_t SpecRxCore::getRxPolarity() {
    return SpecCom::readSingle(RX_ADDR | RX_POLARITY);
}

void SpecRxCore::setRxActiveLanes(uint32_t value) {
    SpecCom::writeSingle(RX_ADDR | RX_ACTIVE_LANES, value);
}

uint32_t SpecRxCore::getRxActiveLanes() {
    return SpecCom::readSingle(RX_ADDR | RX_ACTIVE_LANES);
}

void SpecRxCore::checkRxSync() {
    uint32_t status = this->getLinkStatus();
    uint32_t enable_mask = SpecCom::readSingle(RX_ADDR | RX_ENABLE);
    srxlog->info("Active Rx channels: 0x{:x}", enable_mask);    
    srxlog->info("Active Rx lanes: 0x{:x}", m_rxActiveLanes);
    srxlog->info("Rx Status 0x{:x}", status);
    for (unsigned i=0; i<32; i++) {
        if ((1 << i) & enable_mask) {
            for (unsigned l=0; l<4; l++) {
                if ((1 << l) & m_rxActiveLanes) {
                    if (status & (1 << ((i*8)+l+4))) {
                        srxlog->info("Channel {} Lane {} synchronized!", i, l);
                    } else {
                        srxlog->error("Channel {} Lane {} not synchronized!", i, l);
                    }
                }
            }
        }
    }
}

