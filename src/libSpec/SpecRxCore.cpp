#include <cstring>
#include <algorithm>

#include "SpecRxCore.h"

#include "logging.h"

namespace {
    auto srxlog = logging::make_log("SpecRx");
}

SpecRxCore::SpecRxCore() {
    m_rxActiveLanes = 15; 
}

void SpecRxCore::setRxEnable(uint32_t value) {
    m_rxEnable.clear();
    m_rxEnable.push_back(value);
    uint32_t mask = (1 << value);
    SPDLOG_LOGGER_TRACE(srxlog, "Value {0:x}", mask);
    SpecCom::writeSingle(RX_ADDR | RX_ENABLE, mask);
}

void SpecRxCore::setRxEnable(std::vector<uint32_t> channels) {
    uint32_t mask = 0;
    m_rxEnable = channels;
    std::sort(m_rxEnable.begin(), m_rxEnable.end()); 
    m_rxEnable.erase(std::unique(m_rxEnable.begin(), m_rxEnable.end()), m_rxEnable.end()); 
    for (uint32_t channel : channels) {
        mask |= (1 << channel);

    }
    SPDLOG_LOGGER_TRACE(srxlog, "Value {0:x}", mask);
    SpecCom::writeSingle(RX_ADDR | RX_ENABLE, mask);

}

void SpecRxCore::disableRx() {
    m_rxEnable.clear();
    SPDLOG_LOGGER_TRACE(srxlog, "");
    SpecCom::writeSingle(RX_ADDR | RX_ENABLE, 0x0);
}

void SpecRxCore::maskRxEnable(uint32_t value, uint32_t mask) {
    std::vector<uint32_t> tmpVec;
    for (uint32_t rx : m_rxEnable) {
        if (((1<<rx) & (mask & value)) > 0) {
            tmpVec.push_back(rx);
        }
        if (((1<<rx) & mask) == 0) {
            tmpVec.push_back(rx);
        }
    }
    this->setRxEnable(tmpVec);
}

std::vector<RawDataPtr> SpecRxCore::readData() {
    uint32_t dma_addr = getStartAddr();
    uint32_t dma_count = getDataCount();
    uint32_t real_dma_count = dma_count;
    std::vector<RawDataPtr> dataVec;
    if (dma_count > 0 && dma_count < (251*256)) {
        real_dma_count = dma_count;
        // DMA mapped memory needs to be aligned
        if (dma_count%32 != 0)
            dma_count += 32-(dma_count%32);

        SPDLOG_LOGGER_DEBUG(srxlog, "Read data to Addr 0x{:x}, Count {}", dma_addr, dma_count);
        RawDataPtr data = std::make_shared<RawData>(dma_addr, dma_count);
        if (SpecCom::readDma(dma_addr, data->getBuf(), dma_count)) {
            SPDLOG_LOGGER_CRITICAL(srxlog, "Critical error while readin data ... aborting!!");
            exit(1);
        }
        // Resize to real amount
        data->resize(real_dma_count);
        unsigned channelCount = 0;
        for (uint32_t rx : m_rxEnable) {
            std::shared_ptr<SpecRawData> specData = std::make_shared<SpecRawData>(rx, data);
            specData->setItAndOffset(m_rxEnable.size(), channelCount);
            dataVec.push_back(std::dynamic_pointer_cast<RawData>(specData));
            channelCount++;
        }
    }
    return std::move(dataVec);
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

uint32_t SpecRxCore::setRxDelay(uint32_t lane) {

    uint32_t sel_lane = lane / 2;

    // Select lane 
    SpecCom::writeSingle(RX_ADDR | RX_LANE_SEL, sel_lane); 
    uint32_t current_delay=SpecCom::readSingle(RX_ADDR | RX_LANE_DELAY_OUT);
    uint32_t current_status=this->getLinkStatus(); 
    uint32_t enable_mask = SpecCom::readSingle(RX_ADDR | RX_ENABLE);
    srxlog->info("Locked lane {} on at delay {} with Rx Status {}", sel_lane, current_delay, current_status);

    // Switch on manual delay
    SpecCom::writeSingle(RX_ADDR | RX_MANUAL_DELAY, lane); 

    // Increase delay by 1
    uint32_t new_delay=current_delay+2;
    SpecCom::writeSingle(RX_ADDR | RX_LANE_DELAY, new_delay);   
    current_delay=SpecCom::readSingle(RX_ADDR | RX_LANE_DELAY_OUT);
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    srxlog->info("updated delay {} with Rx Status {:b}", current_delay, current_status);

    return SpecCom::readSingle(RX_ADDR |  RX_LANE_DELAY_OUT);
}

void SpecRxCore::checkRxSync() {
    uint32_t status = this->getLinkStatus();
    uint32_t enable_mask = SpecCom::readSingle(RX_ADDR | RX_ENABLE);
    srxlog->info("Active Rx channels: 0x{:x}", enable_mask);    
    srxlog->info("Active Rx lanes: 0x{:x}", m_rxActiveLanes);
    srxlog->info("Rx Status 0x{:x}", status);
    uint32_t numOfLanes = 0;
    // Convert last digit (lanes) to int
    numOfLanes = this->getSpecIdentLaneCfg(fw_ident);
    if (numOfLanes == 0 || numOfLanes > 4) {
        srxlog->error("Number of Lanes not acceptable: {}", numOfLanes);
        return;
    }
    srxlog->info("Number of lanes: {}", numOfLanes);
    uint32_t delay = 0;
    uint32_t val=0;

    for (unsigned i=0; i<32; i++) {
        if ((1 << i) & enable_mask) {
            for (unsigned l=0; l<4; l++) {
                if ((1 << l) & m_rxActiveLanes) {
                    if (status & (1 << ((i*numOfLanes)+l))) {
                        val=status & (1 << ((i*numOfLanes)+l));
                        srxlog->info("Value {}!", val);
                        delay=SpecRxCore::setRxDelay(val);
                        srxlog->info("Channel {} Lane {} synchronized with delay {}!", i, l, delay);
                    } else {
                        srxlog->error("Channel {} Lane {} not synchronized!", i, l);
                    }
                }
            }
        }
    }


}

