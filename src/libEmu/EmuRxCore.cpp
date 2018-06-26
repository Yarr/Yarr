#if 0

// #################################
// # Author: Timon Heim
// # Email: timon.heim at cern.ch
// # Project: Yarr
// # Description: Emulator Receiver
// # Comment:
// # Date: Jan 2017
// ################################

#include "EmuRxCore.h"
#include <iostream>
#include <unistd.h>
#include <iterator>
#include <iomanip>
#include <algorithm>
#include <chrono>
#include <thread>

#define HEXF(x,y) std::hex << "0x" << std::hex << std::setw(x) << std::setfill('0') << static_cast<int>(y) << std::dec

EmuRxCore::EmuRxCore(EmuCom *com) {
    m_com = com;
}

EmuRxCore::~EmuRxCore() {}

RawData* EmuRxCore::readData() {
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
