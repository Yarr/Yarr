#include "RceRxCore.h"
#include <iostream>
#include <unistd.h>
#include <iterator>
#include <iomanip>
#include <algorithm>
#include <chrono>
#include <thread>

#define HEXF(x,y) std::hex << "0x" << std::hex << std::setw(x) << std::setfill('0') << static_cast<int>(y) << std::dec

RceRxCore::RceRxCore():m_com(0) {
  m_com=new RceCom();
}

RceRxCore::~RceRxCore() { if(m_com) delete m_com;  }




RawData* RceRxCore::readData() {
    //std::this_thread::sleep_for(std::chrono::microseconds(1));
    uint32_t words = this->getCurCount();


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
