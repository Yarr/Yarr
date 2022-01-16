#ifndef BOCRXCORE_H
#define BOCRXCORE_H

// #################################
// # Author: Marius Wensing
// # Email: marius.wensing at cern.ch
// # Project: Yarr
// # Description: Boc Receiver
// # Comment:
// # Data: Mar 2017
// ################################

#include <queue>
#include <vector>
#include <cstdint>
#include "RxCore.h"
#include "BocCom.h"
#include "RawData.h"

class BocRxCore : virtual public RxCore {
    public:
        BocRxCore();
        ~BocRxCore() override;

        void setRxEnable(uint32_t val) override;
        void setRxEnable(std::vector<uint32_t> channels) override;
        void disableRx() override;
        void maskRxEnable(uint32_t val, uint32_t mask) override;

        RawData* readData() override;
        
        uint32_t getDataRate() override;
        uint32_t getCurCount() override;
        bool isBridgeEmpty() override;

        void setEmu(uint32_t mask, uint8_t hitcnt = 0);
        uint32_t getEmu();

        void setCom(BocCom *com) {
            m_com = com;
        }

        BocCom *getCom() {
            return m_com;
        }

    private:

        uint32_t m_enableMask;
        uint32_t m_emuMask;
        std::queue<uint16_t> m_rxData[32];
        uint32_t m_formState[32];
        uint32_t m_formRecord[32];
        BocCom *m_com;
};

#endif
