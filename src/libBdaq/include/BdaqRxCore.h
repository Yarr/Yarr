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
#include <deque>

#include "RxCore.h"
#include "Bdaq.h"
#include "RawData.h"

class BdaqRxCore : virtual public RxCore, virtual public Bdaq {
    public:
        BdaqRxCore();

        void setupMode();
        void runMode();

        void setRxEnable(uint32_t val);
        void setRxEnable(std::vector<uint32_t>);
        void disableRx() {} // Future implementation.
        void maskRxEnable(uint32_t val, uint32_t mask);

        void checkRxSync();
        
        RawData* readData();
        void flushBuffer();
        
        uint32_t getDataRate();
        bool isBridgeEmpty();  
        
        std::chrono::microseconds getWaitTime() {
            return m_waitTime;
        }

        void printBufferStatus();
    
    protected:
        std::chrono::microseconds m_waitTime; 

    private:        
        bool mSetupMode;
        
        std::vector<uint> activeChannels; 
        std::vector<std::deque<uint32_t>> sBuffer;

        unsigned int userkCounter; 
        uint64_t userkWordA, userkWordB;

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

        
        bool isEventHeader;
        bool isHighWord;
        uint32_t dataWord;

        uint counter1 = 0;
        uint counter2 = 0;
        uint counter3 = 0;
        void displaySort();
        
        uint readEqualized();
        void initSortBuffer();
        uint sortChannels(std::vector<uint32_t>& in);
        bool testEqualSize();
        void buildStream(std::vector<uint32_t>& out, uint size);

        unsigned int decode(std::vector<uint32_t>& in, uint32_t* out);
        
        unsigned int decodeUserk(const uint32_t& word, uint32_t* out, 
                                    unsigned int index);

        void buildUserkFrame(const uint32_t& word, unsigned int id);
        
        BdaqRxCore::userkDataT interpretUserkFrame(); 
        
        std::vector<BdaqRxCore::regDataT> getRegData(BdaqRxCore::userkDataT in);

        void encodeToYarr(BdaqRxCore::regDataT in, uint32_t* out, 
                            unsigned int index);

        bool checkTDC(const uint32_t& word);
};

#endif
