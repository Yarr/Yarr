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
#include <fstream>

#define HEXF(x,y) std::hex << "0x" << std::hex << std::setw(x) << std::setfill('0') << static_cast<int>(y) << std::dec

std::fstream rawDataFile("data/rawData/rawData.dat", std::ios::app | std::ios::out | std::ios::binary);

EmuRxCore::EmuRxCore(EmuCom *com) {
    m_com = com;


    /* manages errors */
    if (!rawDataFile){ std::cerr<<"Cannot open the file for buf raw datas"<<std::endl;}
  
}

EmuRxCore::~EmuRxCore() {
  rawDataFile.close();
}

RawData* EmuRxCore::readData() {
    //std::this_thread::sleep_for(std::chrono::microseconds(1));
    uint32_t words = this->getCurCount()/sizeof(uint32_t);
    if (words > 0) {
        uint32_t *buf = new uint32_t[words];
        //for(unsigned i=0; i<words; i++)
        //    buf[i] = m_com->read32();
        if (m_com->readBlock32(buf, words)) {
	    outRawData(0x0, buf, words);
	    return new RawData(0x0, buf, words);
        } else {
	    delete[] buf;
        }
    }
    return NULL;
}


/* ******************************** */
/* Bring out the raw data in a file */
/* ******************************** */

/* adr: idk; buf is an adress in the memory for the data; words is the depth of the data memory */
void EmuRxCore::outRawData(uint32_t adr, uint32_t *buf, uint32_t words){

  using namespace std;

  
  for (int i = 0; i<(int)words; i++){
    rawDataFile.write((char*)buf, sizeof(uint32_t)*words); /* write data in binary */
  }

  

}
