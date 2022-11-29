#ifndef BDAQRXCORE_H
#define BDAQRXCORE_H

// #################################
// # Author: Rafael Gama
// # Email: rafael.gama at cern.ch
// # Project: Yarr
// # Description: BDAQ Receiver Core
// # Comment:
// ################################

#include <iostream>
#include <vector>
#include <queue>

#include "RxCore.h"
#include "Bdaq.h"
#include "RawData.h"

#include "BdaqTxCore.h"

class BdaqRxCore : virtual public RxCore, virtual public Bdaq {
    public:
        BdaqRxCore();

        void setupMode();
        void runMode();

        void setRxEnable(uint32_t val) override;
        void setRxEnable(std::vector<uint32_t>) override;
        void disableRx() override {} // Future implementation.
        void maskRxEnable(uint32_t val, uint32_t mask) override;

        void checkRxSync() override;

        std::vector<RawDataPtr> readData() override;
        std::map<uint32_t, std::vector<uint32_t>> dataMap;
        std::map<uint32_t, std::vector<uint32_t>> dataMap_copy;
        uint32_t dataWord;

        void flushBuffer() override;

        uint32_t getDataRate() override;
        bool isBridgeEmpty() override;


        void printSortStatus();

    protected:
        std::chrono::microseconds m_waitTime;

    private:
        bool mSetupMode;

        std::vector<uint> activeChannels; 
        std::vector<std::queue<uint32_t>> sBuffer;

        struct userkDataT_RD53A {
            uint8_t  AuroraKWord;
            uint8_t  Status;
            uint16_t Data1;
            uint16_t Data1_AddrFlag;
            uint16_t Data1_Addr;
            uint16_t Data1_Data;
            uint16_t Data0;
            uint16_t Data0_AddrFlag;
            uint16_t Data0_Addr;
            uint16_t Data0_Data;
        };

        struct userkDataT_RD53B {
            uint8_t  ChipID = 0;
            uint8_t  AuroraKWord = 0;
            uint8_t  Status = 0;
            uint16_t Data1 = 0;
            uint16_t Data1_AddrFlag = 0;
            uint16_t Data1_Addr = 0;
            uint16_t Data1_Data = 0;
            uint16_t Data0 = 0;
            uint16_t Data0_AddrFlag = 0;
            uint16_t Data0_Addr = 0;
            uint16_t Data0_Data = 0;
        };

        struct regDataT {
            uint16_t Address;
            uint16_t Data;
        };

        void initSortBuffer();

        BdaqRxCore::userkDataT_RD53A interpretUserkFrame_RD53A(uint64_t userkWordA,
                                                    uint64_t userkWordB);
        std::vector<BdaqRxCore::regDataT> getRegData_RD53A(BdaqRxCore::userkDataT_RD53A in);

        BdaqRxCore::userkDataT_RD53B interpretUserkFrame_RD53B(uint64_t userkWordA,
                                                    uint64_t userkWordB);
        std::vector<BdaqRxCore::regDataT> getRegData_RD53B(BdaqRxCore::userkDataT_RD53B in);

        void encodeToYarr(BdaqRxCore::regDataT in, uint32_t* out,
                            unsigned int index);

        bool checkTDC(const uint32_t& word);
};

#endif
