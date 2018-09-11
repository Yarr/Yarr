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
        EmuRxCore(EmuCom *com);
        EmuRxCore() {m_com = NULL;}
        ~EmuRxCore();
        
        void setCom(EmuCom *com) {m_com = com;}
        EmuCom* getCom() {return m_com;}

        void setRxEnable(uint32_t val) {}
        void maskRxEnable(uint32_t val, uint32_t mask) {}

        RawData* readData();
        
        uint32_t getDataRate() {return 0;}
        uint32_t getCurCount() {return m_com->getCurSize();}
        bool isBridgeEmpty() {return m_com->isEmpty();}

    private:
        EmuCom *m_com;
};

template<class FE>
EmuRxCore<FE>::EmuRxCore(EmuCom *com) {
    m_com = com;
}

template<class FE>
EmuRxCore<FE>::~EmuRxCore() {}

template<class FE>
RawData* EmuRxCore<FE>::readData() {
    //std::this_thread::sleep_for(std::chrono::microseconds(1));
    uint32_t words = this->getCurCount()/sizeof(uint32_t);
    if (words > 0) {
        uint32_t *buf = new uint32_t[words];
        //for(unsigned i=0; i<words; i++)
        //    buf[i] = m_com->read32();
        if (m_com->readBlock32(buf, words)) {
            return new RawData(0x0, buf, words);
        } else {
            delete[] buf;
        }
    }
    return NULL;
}
#endif
