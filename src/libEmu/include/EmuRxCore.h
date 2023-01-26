#ifndef EMURXCORE_H
#define EMURXCORE_H

// #################################
// # Author: Timon Heim
// # Email: timon.heim at cern.ch
// # Project: Yarr
// # Description: Emulator Transmitter 
// # Comment: 
// # Date: Jan 2017
// ################################

#include <iostream>

#include "RxCore.h"
#include "EmuCom.h"

template<class FE>
class EmuRxCore : virtual public RxCore {
    public:
        EmuRxCore() = default;
        ~EmuRxCore() override;
        
        void setCom(uint32_t chn, EmuCom *com);
        EmuCom* getCom(uint32_t chn);

        void setRxEnable(uint32_t channel) override;
        void setRxEnable(std::vector<uint32_t> channels) override;
        void maskRxEnable(uint32_t val, uint32_t mask) override {}
        void disableRx() override;
        void enableRx();
        std::vector<uint32_t> listRx();

        std::vector<RawDataPtr> readData() override;
        RawDataPtr readData(uint32_t chn);
        
        uint32_t getDataRate() override {return 0;}

        uint32_t getCurCount(uint32_t chn) {return m_coms[chn]->getCurSize();}
        uint32_t getCurCount() override {
            uint32_t cnt = 0;
            for (auto& com : m_coms) {
                if (m_channels[com.first]) cnt += com.second->getCurSize();
            }
            return cnt;
        }

        bool isBridgeEmpty() override {
            for (auto& com : m_coms) {
                if (m_channels[com.first])
                    if (not com.second->isEmpty()) return false;
            }
            return true;
        }

    private:
        std::map<uint32_t, EmuCom*> m_coms;
        std::map<uint32_t, bool> m_channels;
};

template<class FE>
EmuRxCore<FE>::~EmuRxCore() = default;

template<class FE>
void EmuRxCore<FE>::setCom(uint32_t chn, EmuCom *com) {
    m_coms[chn] = com;
    m_channels[chn] = true;
}

template<class FE>
EmuCom* EmuRxCore<FE>::getCom(uint32_t chn) {
    if (m_coms.find(chn) != m_coms.end()) {
        return m_coms[chn];
    } else {
        return nullptr;
    }
}

template<class FE>
RawDataPtr EmuRxCore<FE>::readData(uint32_t chn) {
    RawDataPtr data;
    std::vector<uint32_t> in;
    m_coms[chn]->readAll(in);
    data = std::make_shared<RawData>(chn, std::move(in));
    return std::move(data);
}

template<class FE>
std::vector<RawDataPtr> EmuRxCore<FE>::readData() {
    std::vector<RawDataPtr> dataVec;
    for (auto& com : m_coms) {
        if (not m_channels[com.first]) continue;
        if (com.second->isEmpty()) continue;
        dataVec.push_back(this->readData(com.first));
    }
    return dataVec;
}

template<class FE>
void EmuRxCore<FE>::setRxEnable(uint32_t channel) {
    // disable all channels first
    this->disableRx();

    // check if the channel exists
    if (m_coms.find(channel) != m_coms.end())
        m_channels[channel] = true;
    //else
        //logger->warn("Channel {} has not been configured!", channel);
}

template<class FE>
void EmuRxCore<FE>::setRxEnable(std::vector<uint32_t> channels) {
    this->disableRx();
    for (auto channel : channels) {
        if (m_coms.find(channel) != m_coms.end())
            m_channels[channel] = true;
    }
}

template<class FE>
void EmuRxCore<FE>::enableRx() {
    for (auto& com : m_coms) {
        m_channels[com.first] = true;
    }
}

template<class FE>
void EmuRxCore<FE>::disableRx() {
    for (auto& com : m_coms) {
        m_channels[com.first] = false;
    }
}

template<class FE>
std::vector<uint32_t> EmuRxCore<FE>::listRx() {
    std::vector<uint32_t> rxChannels;
    rxChannels.reserve(m_coms.size());
    for (auto& com: m_coms) {
        rxChannels.push_back(com.first);
    }
    return rxChannels;
}
#endif
