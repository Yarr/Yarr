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



// TODO this does not work, it just compiles
std::vector<std::shared_ptr<RawData>> RogueRxCore::readData() {
	m_com->setRxChannel(m_rxChannel);
    std::this_thread::sleep_for(std::chrono::microseconds(10));
    uint32_t words = this->getCurCount();

    std::vector<std::shared_ptr<RawData>> dataVec;

    if (words > 0) {
        // TODO need to assign channel
        std::shared_ptr<RawData> data = std::make_shared<RawData>(0x0, words);
        uint32_t *buf = data->getBuf();
        if (m_com->readBlock32(buf, words)) {
            dataVec.push_back(data);
        }
    }
    return dataVec;
}
std::shared_ptr<RogueCom> RogueCom::instance;
