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

void outRawData(uint32_t adr, uint32_t *buf, uint32_t words);

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
void outRawData(uint32_t adr, uint32_t *buf, uint32_t words){

  using namespace std;

  ofstream rawDataFile; /* the file need to be opened in binary, not in ASCII */
  rawDataFile.open("data/rawData/rawData.csv", ios::app | ios::out | ios::binary); /* app is for appending ! */

  /* manages errors */
  if (!rawDataFile){ cerr<<"Cannot open the file for buf raw datas"<<endl;}
  
  for (int i = 0; i<(int)words; i++){
    rawDataFile << hex << *(buf + i) << ",";
  }

  rawDataFile.close();

}
