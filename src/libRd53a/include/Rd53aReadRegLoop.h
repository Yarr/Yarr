#ifndef RD53AREADREGLOOP_H
#define RD53AREADREGLOOP_H

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

class Rd53aReadRegLoop : public LoopActionBase {

 public:
  Rd53aReadRegLoop();
  

  void writeConfig(json &config);
  void loadConfig(json &config);

  
 private:

  std::vector<unsigned short> VoltMux;
  std::vector<unsigned short> CurMux;
  std::vector<unsigned short> TempMux;

  uint16_t ReadADC(unsigned short Reg, bool doCur);
  uint16_t ReadTemp(unsigned short Reg);


  void init();
  void execPart1();
  void execPart2();
  void end();

};

#endif
