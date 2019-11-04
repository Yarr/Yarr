// #################################
// # Author: Ismet Siral
// # Email: ismet.siral at cern.ch
// # Project: Yarr
// # Description: Read Register Loop for RD53A
// # Date: 03/2018
// ################################

#include "Rd53aRingBufLoop.h"


Rd53aRingBufLoop::Rd53aRingBufLoop() {
    loopType = typeid(this);
    m_EnblRingOsc=0;
    m_RstRingOsc=0;
    m_RingOscDur=0;



}



uint16_t Rd53aRingBufLoop::ReadRegister(Rd53aReg Rd53aGlobalCfg::*ref,  Rd53a *tmpFE = NULL) {
     
  if(tmpFE==NULL)
    tmpFE= keeper->globalFe<Rd53a>();

  g_rx->flushBuffer();
  tmpFE->readRegister(ref);
 
  std::this_thread::sleep_for(std::chrono::microseconds(500));

  RawData *data = g_rx->readData();
  if(!data)
    {
      std::cout<<"Warning!!!, No Word Recieved in ReadRegister"<<std::endl;
      return 65535;
    }
  unsigned size =  data->words;
  //std::cout<<"Word cout is: "<<size<<std::endl;
  for(unsigned c=0; c<size/2; c++)
    {
      if (c*2+1<size) {
	//return decodeSingleRegRead(data->buf[c*2],data->buf[c*2+1]).second;	    
	std::pair<uint32_t, uint32_t> readReg = decodeSingleRegRead(data->buf[c*2],data->buf[c*2+1]);	    
	if ( readReg.first==(tmpFE->*ref).addr())
	  return readReg.second;
      }
      else {
	std::cout<<"Warning!!! Hallfword recieved in ADC Register Read"<<data->buf[c*2]<<std::endl;
	return 65535;
      }
    }
  return 65535;
}




void Rd53aRingBufLoop::init() {
    if (verbose)
        std::cout << __PRETTY_FUNCTION__ << std::endl;
    m_done = false;
    



}

void Rd53aRingBufLoop::execPart1() {


  dynamic_cast<HwController*>(g_rx)->setupMode(); //This is need to make sure the global rests doesn't refresh the ADCRegister
  
  keeper->globalFe<Rd53a>()->writeRegister(&Rd53a::RingOscEn, m_EnblRingOsc);

  for(uint16_t tmpCount = 0; tmpCount<8; tmpCount++) {
    if ( ((m_RstRingOsc >> tmpCount) % 2) == 1 && ((m_EnblRingOsc >> tmpCount) % 2) == 1){
      keeper->globalFe<Rd53a>()->writeRegister(OscRegisters[tmpCount],0);
    }
  }

  std::cout<<"Starting Ring Osc"<<std::endl;
  
  for (auto *fe : keeper->feList) {
    if(fe->getActive()) {
      for(uint16_t tmpCount = 0; tmpCount<8 ; tmpCount++) { 
	if ( ((m_EnblRingOsc >> tmpCount) % 2) == 1) 	
	  std::cout<<"RigBuffer "<<tmpCount<<" For FE "<<dynamic_cast<FrontEndCfg*>(fe)->getTxChannel()<<" At Start: "<<ReadRegister(OscRegisters[tmpCount],dynamic_cast<Rd53a*>(fe))<<std::endl; 
      }
    }
  }

  keeper->globalFe<Rd53a>()->RunRingOsc(m_RingOscDur);

  for (auto *fe : keeper->feList) {
    if(fe->getActive()) {
      for(uint16_t tmpCount = 0; tmpCount<8; tmpCount++) {    
	if ( ((m_EnblRingOsc >> tmpCount) % 2) == 1) 		
	  std::cout<<"RigBuffer For FE: "<<dynamic_cast<FrontEndCfg*>(fe)->getTxChannel()<<" At End: "<<ReadRegister(OscRegisters[tmpCount],dynamic_cast<Rd53a*>(fe))<<std::endl; 
      }
    }
  }
    
  dynamic_cast<HwController*>(g_rx)->runMode(); //This is needed to revert back the setupMode
  
}

void Rd53aRingBufLoop::execPart2() {
  m_done = true;
}

void Rd53aRingBufLoop::end() {
    // Reset to min
}

void Rd53aRingBufLoop::writeConfig(json &config) {
  config["EnblRingOsc"] = m_EnblRingOsc;
  config["RstRingOsc"] = m_RstRingOsc;
  config["RingOscDur"] = m_RingOscDur;
    
    
}

void Rd53aRingBufLoop::loadConfig(json &config) {

  if (!config["EnblRingOsc"].empty())
    m_EnblRingOsc = config["EnblRingOsc"];
  if (!config["RstRingOsc"].empty())
    m_RstRingOsc = config["RstRingOsc"];
  if (!config["RingOscDur"].empty())
    m_RingOscDur = config["RingOscDur"];

}





















