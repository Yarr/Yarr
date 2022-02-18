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
        
        std::shared_ptr<RawData> readData() override;
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
                
        struct userkDataT {
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

        struct regDataT {
            uint16_t Address;
            uint16_t Data;
        };
               
        void initSortBuffer();
        uint sortChannels(std::vector<uint32_t>& in);
        void buildData(uint32_t* out, uint bIndex, uint oIndex);
        void buildUserk(uint32_t* out, uint bIndex, uint oIndex);
        uint buildStream(uint32_t* out, uint size);
                
        BdaqRxCore::userkDataT interpretUserkFrame(uint64_t userkWordA, 
                                                    uint64_t userkWordB); 
        std::vector<BdaqRxCore::regDataT> getRegData(BdaqRxCore::userkDataT in);
        void encodeToYarr(BdaqRxCore::regDataT in, uint32_t* out, 
                            unsigned int index);

        bool checkTDC(const uint32_t& word);
};

#endif
