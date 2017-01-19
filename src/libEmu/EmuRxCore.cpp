// #################################
// # Author: Timon Heim
// # Email: timon.heim at cern.ch
// # Project: Yarr
// # Description: Emulator Receiver
// # Comment:
// # Date: Jan 2017
// ################################

#include "EmuRxCore.h"

EmuRxCore::EmuRxCore(EmuCom *com) {
    m_com = com;
}

EmuRxCore::~EmuRxCore() {}

RawData* EmuRxCore::readData() {
    uint32_t count = this->getCurCount();
    if (count > 0) {
        uint32_t *buf = new uint32_t[count];
        for(unsigned i=0; i<count; i++)
            buf[i] = m_com->read32();
        return new RawData(0x0, buf, count*4);
    } else {
        return NULL;
    }
    return NULL;
}
