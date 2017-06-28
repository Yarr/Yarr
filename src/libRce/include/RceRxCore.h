#ifndef RCERXCORE_H
#define RCERXCORE_H


#include <iostream>

#include "RxCore.h"
#include "RceCom.h"

class RceRxCore : virtual public RxCore {
    public:
        RceRxCore();
        ~RceRxCore();
        
        void setRxEnable(uint32_t val) {}
        void maskRxEnable(uint32_t val, uint32_t mask) {}

        RawData* readData();
        
        uint32_t getDataRate() {return 0;}
        uint32_t getCurCount() {return m_com->getCurSize();}
        bool isBridgeEmpty() {return m_com->isEmpty();}

    private:
        RceCom *m_com;
};

#endif
