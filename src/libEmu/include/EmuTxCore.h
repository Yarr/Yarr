#ifndef EMUTXCORE_H
#define EMUTXCORE_H

// #################################
// # Author: Timon Heim
// # Email: timon.heim at cern.ch
// # Project: Yarr
// # Description: Emulator Transmitter 
// # Comment: 
// # Date: Jan 2017
// ################################

#include <atomic>
#include <iostream>
#include <thread>
#include <mutex>
#include <map>

#include "TxCore.h"
#include "EmuCom.h"

template<class FE>
class EmuTxCore : virtual public TxCore {
    public:
        EmuTxCore();
        ~EmuTxCore() override;

        void setCom(uint32_t chn, EmuCom *com);
        EmuCom* getCom(uint32_t chn);

        void writeFifo(uint32_t value) override;
        void writeFifo(uint32_t chn, uint32_t value);
        void releaseFifo() override {this->writeFifo(0x0);} // Add some padding

        void setCmdEnable(uint32_t channel) override;
        void setCmdEnable(std::vector<uint32_t> channels) override;
        void disableCmd() override;
        void enableCmd();
        std::vector<uint32_t> listTx();
        uint32_t getCmdEnable() override {return 0x0;}
        void maskCmdEnable(uint32_t value, uint32_t mask) {}

        void setTrigEnable(uint32_t value) override;
        uint32_t getTrigEnable() override {return triggerProc.joinable() || !this->isCmdEmpty();}
        void maskTrigEnable(uint32_t value, uint32_t mask) override {}

        void setTrigConfig(enum TRIG_CONF_VALUE cfg) override {}
        void setTrigFreq(double freq) override {}
        void setTrigCnt(uint32_t count) override;
        void setTrigTime(double time) override {}
        
        void setTrigWordLength(uint32_t length) override { trigLength = length; }
        void setTrigWord(uint32_t *word, uint32_t length) override { trigWord = word; trigLength = length; }

        void toggleTrigAbort() override {}

        bool isCmdEmpty() override {
            for (auto& com : m_coms) {
                if (m_channels[com.first])
                    if (not com.second->isEmpty()) return false;
            }
            return true;
        }

        bool isTrigDone() override {
            bool rtn = !trigProcRunning && this->isCmdEmpty();
            return rtn;
        }

        uint32_t getTrigInCount() override {return 0x0;}
        
        void setTriggerLogicMask(uint32_t mask) override {}
        void setTriggerLogicMode(enum TRIG_LOGIC_MODE_VALUE mode) override {}
        void resetTriggerLogic() override {}

    private:
        std::map<uint32_t, EmuCom*> m_coms;
        std::map<uint32_t, bool> m_channels;

        unsigned m_trigCnt;
        std::mutex accMutex;
        std::thread triggerProc;
        std::atomic<bool> trigProcRunning;
    uint32_t* trigWord;
    uint32_t trigLength;
        void doTrigger();
};

template<class FE>
EmuTxCore<FE>::EmuTxCore() {
    m_trigCnt = 0;
    trigProcRunning = false;
}

template<class FE>
EmuTxCore<FE>::~EmuTxCore() = default;

template<class FE>
void EmuTxCore<FE>::setCom(uint32_t chn, EmuCom *com) {
    m_coms[chn] = com;
    m_channels[chn] = true;
}

template<class FE>
EmuCom* EmuTxCore<FE>::getCom(uint32_t chn) {
    if (m_coms.find(chn) != m_coms.end()) {
        return m_coms[chn];
    } else {
        return nullptr;
    }
}

template<class FE>
void EmuTxCore<FE>::writeFifo(uint32_t value) {
    for (auto& chn_en : m_channels) {
        if (chn_en.second) {
            this->writeFifo(chn_en.first, value);
        }
    }
}

template<class FE>
void EmuTxCore<FE>::writeFifo(uint32_t chn, uint32_t value) {
    if (m_channels[chn]) m_coms[chn]->write32(value);
}

template<class FE>
void EmuTxCore<FE>::setTrigCnt(uint32_t count) {
    m_trigCnt = count;
}

template<class FE>
void EmuTxCore<FE>::setTrigEnable(uint32_t value) {
    // TODO value should reflect channel
    if(value == 0) {
        if (triggerProc.joinable()) triggerProc.join();
    } else {
        trigProcRunning = true;
        triggerProc = std::thread(&EmuTxCore::doTrigger, this);
    }
}

template<class FE>
void EmuTxCore<FE>::setCmdEnable(uint32_t channel) {
    // disable all channels first
    this->disableCmd();

    // enable channel
    // check if the channel exists
    if (m_coms.find(channel) != m_coms.end())
        m_channels[channel] = true;
    //else
        //logger->warn("Channel {} has not been configured!", channel);
}

template<class FE>
void EmuTxCore<FE>::setCmdEnable(std::vector<uint32_t> channels) {
    this->disableCmd();
    for (auto channel : channels) {
        if (m_coms.find(channel) != m_coms.end())
            m_channels[channel] = true;
    }
}

template<class FE>
void EmuTxCore<FE>::enableCmd() {
    for (auto& com : m_coms) {
        m_channels[com.first] = true;
    }
}

template<class FE>
void EmuTxCore<FE>::disableCmd() {
    for (auto& com : m_coms) {
        m_channels[com.first] = false;
    }
}

template<class FE>
std::vector<uint32_t> EmuTxCore<FE>::listTx() {
    std::vector<uint32_t> txChannels;
    txChannels.reserve(m_coms.size());
    for (auto& com : m_coms) {
        txChannels.push_back(com.first);
    }
    return txChannels;
}

#endif
