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
    std::cout << __PRETTY_FUNCTION__ << std::endl;
    m_com = com;
}

EmuRxCore::~EmuRxCore() {}

RawData* EmuRxCore::readData() {
    std::this_thread::sleep_for(std::chrono::microseconds(10));
    uint32_t words = this->getCurCount()/sizeof(uint32_t);
    if (words > 0) {
        uint32_t *buf = new uint32_t[words];
        for(unsigned i=0; i<words; i++)
            buf[i] = m_com->read32();
#if 0
	std::cout << __PRETTY_FUNCTION__ << ": words=" << words << " -- ";
	std::cout << std::hex;
	std::for_each(buf,buf+words,[](uint32_t w){ std::cout << HEXF(8,w) << "\t"; });
	//std::copy(buf,buf+words,std::ostream_iterator<uint32_t>(std::cout,"\t"));
	std::cout << std::dec << std::endl;
#endif
        return new RawData(0x0, buf, words);
    }
    return NULL;
}
