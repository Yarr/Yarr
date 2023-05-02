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
#define RX_POLARITY 0x2
#define RX_ACTIVE_LANES 0x3
#define RX_LANE_SEL 0x4
#define RX_LANE_DELAY 0x5
#define RX_MANUAL_DELAY 0x6
#define RX_LANE_DELAY_OUT 0x7

#define RX_START_ADDR 0x0
#define RX_DATA_COUNT 0x1
#define RX_LOOPBACK 0x2
#define RX_DATA_RATE 0x3
#define RX_LOOP_FIFO 0x4
#define RX_BRIDGE_EMPTY 0x5
#define RX_CUR_COUNT 0x6

class SpecRawData : public RawData {
    public:
        SpecRawData(uint32_t arg_adr, RawDataPtr arg_data) : RawData(0, 0) {
            adr = arg_adr;
            data = arg_data;
        }

        ~SpecRawData()=default;

        inline void resize(unsigned arg_words) override {
            data->resize(arg_words);
        }

        inline uint32_t& getAdr() override {
            return adr;
        }
        
        void setItAndOffset(uint32_t activeChannels, uint32_t channel) {
            m_activeChannels = activeChannels;
            m_channel = channel;
        }
        
        inline unsigned getSize() const override {
            return data->getSize()/m_activeChannels;
        }

        inline uint32_t& operator [](size_t i) override {
            return data->get(((i/2)*m_activeChannels*2)+(m_channel*2)+(i%2));
        }

        inline uint32_t& get(size_t i) override {
            return data->get(((i/2)*m_activeChannels*2)+(m_channel*2)+(i%2));
        }
    private:
        uint32_t m_activeChannels{1};
        uint32_t m_channel{0};
        RawDataPtr data;
};

class SpecRxCore : virtual public RxCore, virtual public SpecCom{
    public:
        SpecRxCore();

        void setRxEnable(uint32_t val) override;
        void setRxEnable(std::vector<uint32_t> channels) override;
        void disableRx() override;
        void maskRxEnable(uint32_t val, uint32_t mask) override;

        std::vector<RawDataPtr > readData() override;
        void flushBuffer() override;
        
        uint32_t getDataRate() override;
        uint32_t getCurCount() override;
        bool isBridgeEmpty() override;
        
        uint32_t getLinkStatus();
        
        void setRxPolarity(uint32_t val);
        uint32_t getRxPolarity();

        void setRxActiveLanes(uint32_t val);
        uint32_t getRxActiveLanes();

        uint32_t setRxDelay(uint32_t lane);

        void checkRxSync() override;

    protected:
        uint32_t m_rxActiveLanes;
        uint32_t m_rxDelayOffset;

    private:
        uint32_t getStartAddr();
        uint32_t getDataCount();
        std::vector<uint32_t> m_rxEnable;


};

#endif
