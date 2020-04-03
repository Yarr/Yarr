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

#include "RxCore.h"
#include "Bdaq53.h"
#include "RawData.h"

class BdaqRxCore : virtual public RxCore, virtual public Bdaq53 {
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
        
        void printBufferStatus();

    private:
        bool verbose;
        bool mSetupMode;        
        
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
        
        unsigned int decode(std::vector<uint32_t>& in, uint32_t* out);
        
        unsigned int decodeUserk(const uint32_t& word, uint32_t* out, 
                                    unsigned int index);

        void buildUserkFrame(const uint32_t& word, unsigned int id);
        
        BdaqRxCore::userkDataT interpretUserkFrame(); 
        
        std::vector<BdaqRxCore::regDataT> getRegData(BdaqRxCore::userkDataT in);

        void encodeToYarr(BdaqRxCore::regDataT in, uint32_t* out, 
                            unsigned int index);

        bool checkTDC(const uint32_t& word);

        void printStats();

        //debug
        unsigned long totalWords = 0;
        unsigned long userkWords = 0;
        unsigned long dataWords = 0;
        unsigned long headerWords = 0;
        unsigned long hitWords = 0;
};

#endif
