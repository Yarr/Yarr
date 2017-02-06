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

#endif
