#include "RogueRxCore.h"
#include <iostream>
#include <unistd.h>
#include <iterator>
#include <iomanip>
#include <algorithm>
#include <chrono>
#include <thread>

#define HEXF(x,y) std::hex << "0x" << std::hex << std::setw(x) << std::setfill('0') << static_cast<int>(y) << std::dec

RogueRxCore::RogueRxCore():m_com(0), m_rxChannel(0) {
  m_com=RogueCom::getInstance();
}

RogueRxCore::~RogueRxCore() = default;




RawData* RogueRxCore::readData() {
	m_com->setRxChannel(m_rxChannel);
    std::this_thread::sleep_for(std::chrono::microseconds(10));
    uint32_t words = this->getCurCount();


    if (words > 0) {
        uint32_t *buf = new uint32_t[words];
        if (m_com->readBlock32(buf, words)) {
            return new RawData(0x0, buf, words);
        } else {
            delete[] buf;
        }
    }
    return NULL;
}
std::shared_ptr<RogueCom> RogueCom::instance;
