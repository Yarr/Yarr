#ifndef RD53ARINGBUFLOOP_H
#define RD53ARINGBUFLOOP_H

// #################################
// # Author: Ismet Siral
// # Email: ismet.siral at cern.ch
// # Project: Yarr
// # Description: Loop for Reading Registers for RD53A
// # Date: 02/2018
// ################################

#include <array>
#include <chrono>
#include <thread>
#include "LoopActionBase.h"
#include "Rd53a.h"
#include "Rd53aCmd.h"

class Rd53aRingBufLoop : public LoopActionBase {

 public:
  Rd53aRingBufLoop();
  

  void writeConfig(json &config);
  void loadConfig(json &config);

  
 private:

  std::vector<unsigned short> VoltMux;
  std::vector<unsigned short> CurMux;
  std::vector<unsigned short> TempMux;
 
  uint16_t ReadRegister(Rd53aReg Rd53aGlobalCfg::*ref,  Rd53a *tmpFE );
  
  uint16_t m_EnblRingOsc,m_RstRingOsc,m_RingOscDur;


  void init();
  void execPart1();
  void execPart2();
  void end();

  Rd53aReg Rd53aGlobalCfg::* OscRegisters[8] = {&Rd53a::RingOsc0,&Rd53a::RingOsc1,&Rd53a::RingOsc2,&Rd53a::RingOsc3,&Rd53a::RingOsc4,&Rd53a::RingOsc5,&Rd53a::RingOsc6,&Rd53a::RingOsc7};


};

#endif
