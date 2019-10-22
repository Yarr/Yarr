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
  std::string m_RegisterName;
  //Rd53aReg Rd53aGlobalCfg::m_Register;
  Rd53aReg Rd53aGlobalCfg::*m_AdcRead;
  Rd53aReg Rd53aGlobalCfg::*m_sensorConf99;
  Rd53aReg Rd53aGlobalCfg::*m_sensorConf100;
  uint32_t m_MVS;
  uint32_t m_MIS;

  std::vector<unsigned short> NormalMux;
  std::vector<unsigned short> TempMux;




  void (Rd53aReadRegLoop::*ADCFunc)();


  void ReadTemp(unsigned short Reg);
  void ReadADC(unsigned short Reg);

  void init();
  void execPart1();
  void execPart2();
  void end();

};

#endif
