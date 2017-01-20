/*
 * Author: K. Potamianos <karolos.potamianos@cern.ch>
 * Date: 2016-VI-25
 * Description: this is a port of the FE-I4 emulator for IBLROD (2014-VI-12)
 */

#include "Fei4Emu.h"

Fei4Emu::Fei4Emu() {
  m_feId = 0xaa;
  m_feId = 0x00;
  m_feStream.reserve(1024);
  m_feGeo = { 336, 80 };
}

Fei4Emu::~Fei4Emu() {

  std::cout << "m_inStream.size()=" << m_inStream.size() << std::endl;
  for(auto& w : m_inStream) std::cout << std::hex << w << std::dec << std::endl;
}

// Assuming that first command byte is 0x01 (e.g. triger command is 0x1d0)
// Assuming that function isn't called until data is available to be read
// todo: use some buffering with size information (and command availability)

void Fei4Emu::decodeCommand(uint8_t *cmdStream, std::size_t size) {
  std::cout << __PRETTY_FUNCTION__ << std::endl;

  uint8_t *cmdPtr = cmdStream;
  while(cmdPtr < cmdStream+size) {
    while (cmdPtr[0] != 0x1) cmdPtr++; // Look for 0x1
    switch(cmdPtr[1] & 0xF0) {
      case 0xd0: processL1A(); break;
      case 0x60: {
        switch(cmdPtr[1] & 0x0F) {
          case 0x1: processBCR(); break;
          case 0x2: processECR(); break;
          case 0x4: processCAL(); break;
          case 0x8: processSLOW(cmdPtr); break;
        }
        }; break;
      default: {
        std::cerr << "Fei4Emu: cannot decode command" << std::endl;
        std::for_each(cmdPtr, cmdPtr+4, [](uint8_t cmdByte) { std::cerr << std::hex << static_cast<uint32_t>(cmdByte) << std::dec << " "; });
        }
    }
    cmdPtr += 2;
  }

}

///
/// Randomly adding hits
/// Todo: handle only enabled pixels
///

void Fei4Emu::addRandomHits(uint32_t nHits) {
  startFrame();
  addDataHeader(false);
  addServiceRecord(true); // True because we can insert SRs [14-16] here
  while(nHits-- > 0) {
    addDataRecord( rand() % m_feGeo.nCol, rand() % m_feGeo.nRow, rand() % 0xF, rand() % 0xF);
  }
  addServiceRecord(false);
  endFrame();
}

void Fei4Emu::addPhysicsHits() {
}

void Fei4Emu::addServiceRecord(bool isInfoSR) {

}

void Fei4Emu::startFrame() {
  pushOutput( (m_feId << 24) | 0xfc );
}

void Fei4Emu::endFrame() {
  pushOutput( (m_feId << 24) | 0xbc );
}

void Fei4Emu::addDataHeader(bool hasErrorFlags) {
  pushOutput( (m_feId << 24) | (0xe9 << 16) | (((uint32_t)hasErrorFlags) << 15) | ((l1IdCnt&0x1F) << 10) | (bcidCnt&0x3FF) );
}

void Fei4Emu::addDataRecord(uint16_t col, uint16_t row, uint8_t tot1, uint8_t tot2) {
  pushOutput( (m_feId << 24) | ((col&0x7F) << 17) | ((row&0x1FF) << 8) | ((tot1&0xF) << 4) | (tot2&0xF) );
}

void Fei4Emu::addAddressRecord(uint16_t address, bool isGR) {
  pushOutput( (0xea << 16) | (((uint32_t)isGR) << 15) | (address&0x7FFFF) );
}

void Fei4Emu::addValueRecord(uint16_t value) {
  pushOutput( (0xec << 16) | value );
}

void Fei4Emu::processL1A() {
  std::cout << __PRETTY_FUNCTION__ << std::endl;
  addRandomHits(10);
}

void Fei4Emu::processBCR() {
  std::cout << __PRETTY_FUNCTION__ << std::endl;

}

void Fei4Emu::processECR() {
  std::cout << __PRETTY_FUNCTION__ << std::endl;

}

void Fei4Emu::processCAL() {
  std::cout << __PRETTY_FUNCTION__ << std::endl;

}

void Fei4Emu::processSLOW(uint8_t *cmdPtr) {
  std::cout << __PRETTY_FUNCTION__ << std::endl;

}
