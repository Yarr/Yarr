/*
 * Author: K. Potamianos <karolos.potamianos@cern.ch>
 * Date: 2016-VI-25
 * Description: this is a port of the FE-I4 emulator for IBLROD (2014-VI-12)
 */

#ifndef __FEI4_EMU_H__
#define __FEI4_EMU_H__

#define HEXF(x,y) std::hex << "0x" << std::hex << std::setw(x) << std::setfill('0') << static_cast<int>(y) << std::dec

#include "Fei4Cfg.h"
#include "FrontEndGeometry.h"
#include "EmuShm.h"

#include <cstdint>

struct Fei4Emu {
  Fei4Emu();
  ~Fei4Emu();

  void writeFifo(uint32_t value) { m_inStream.push_back(value); }

  /// Acesssors to read internal FE config
  /// (allows faster transmission until command decoder is fully validated)
  void setFeConfig(Fei4Cfg& cfg) { m_feCfg = cfg; }
  const Fei4Cfg& getFeConfig() const { return m_feCfg; }

  const std::vector<uint32_t>& getFeStream() const { return m_feStream; }
  void clearFeStream() { m_feStream.clear(); }

  // Call getFeStream() to see response from FE
  void decodeCommand(uint8_t* cmdStream, std::size_t size);

  // Only public function so far: to be moved to private once command decoder is fully implemented
  void addRandomHits(uint32_t nHits);

  void addPhysicsHits();

  void pushOutput(uint32_t value) {
   //std::cout << __PRETTY_FUNCTION__ << ": " << HEXF( 8, value ) << std::endl;
   m_feStream.push_back(value);
   if(m_rxShMem) m_rxShMem->write32(value); 
  }

  //todo: REMOVE ME ASAP, REALLY, ASAP
  void setRxShMem(std::shared_ptr<EmuShm> rxShMem) {
    m_rxShMem = rxShMem;
  }
  std::shared_ptr<EmuShm> m_rxShMem;

  /// Adds hit to output
  /// @tot1, @tot2: expressed in "real" terms (connected to ToT code later)
  void addHit(uint16_t col, uint16_t row, uint8_t tot1, uint8_t tot2);

//private:

  uint8_t getToTCode(uint8_t dec_tot);

  void addServiceRecord(bool isInfoSR);
  void startFrame();
  void endFrame();
  void addDataHeader(bool hasErrorFlags);
  void addDataRecord(uint16_t col, uint16_t row, uint8_t tot1, uint8_t tot2);
  void addAddressRecord(uint16_t address, bool isGR);
  void addValueRecord(uint16_t value);

  void processL1A();
  void processBCR();
  void processECR();
  void processCAL();
  void processSLOW(uint8_t *cmdPtr);

  std::vector<uint32_t> m_inStream;
  std::vector<uint32_t> m_feStream;
  Fei4Cfg m_feCfg;

  FrontEndGeometry m_feGeo; // todo: put as template arguments (or at least tie to config type
  uint8_t m_feId; // Dummy ID to return in the format (todo: link to something else)

  uint32_t l1IdCnt;
  uint32_t bcidCnt;
};

#endif //__FEI4_EMU_H__
