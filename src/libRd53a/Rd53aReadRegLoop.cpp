// #################################
// # Author: Ismet Siral
// # Email: ismet.siral at cern.ch
// # Project: Yarr
// # Description: Read Register Loop for RD53A
// # Date: 03/2018
// ################################

#include "Rd53aReadRegLoop.h"


Rd53aReadRegLoop::Rd53aReadRegLoop() {
    loopType = typeid(this);
    m_EnblRingOsc=0;
    m_RstRingOsc=0;
    m_RingOscDur=0;


}

uint16_t Rd53aReadRegLoop::ReadRegister(Rd53aReg Rd53aGlobalCfg::*ref,  Rd53a *tmpFE = NULL) {
     
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
	std::pair<uint32_t, uint32_t> readReg = decodeSingleRegRead(data->buf[c*2],data->buf[c*2+1]);	    
	if ( readReg.first==(tmpFE->*ref).addr())
	  return readReg.second;
      }
      else {
	std::cout<<"Warning!!! Hallfword recieved in ADC Register Read "<<data->buf[c*2]<<std::endl;
	return 65535;
      }
    }

  std::cout<<"Warning!!! Requested Register "<<(tmpFE->*ref).addr()<<" not found in recieved words"<<std::endl; 
  return 65535;
}

//Configures the ADC, reads the register returns the first recieved register.
uint16_t Rd53aReadRegLoop::ReadADC(unsigned short Reg,  bool doCur=false,  Rd53a *tmpFE = NULL) {
     
  if(tmpFE==NULL)
    tmpFE= keeper->globalFe<Rd53a>();

  tmpFE->confADC(Reg,doCur);
  return ReadRegister(&Rd53a::AdcRead, dynamic_cast<Rd53a*>(tmpFE));
}

//Runs readADC twice, for two difference bias configurations in the temp/rad sensors. Returns the difference to the user. 
std::pair<uint16_t,uint16_t> Rd53aReadRegLoop::ReadTemp(unsigned short Reg, Rd53a *tmpFE = NULL) {

  //Sensor Config
  if(tmpFE==NULL)
    tmpFE= keeper->globalFe<Rd53a>();



  uint16_t ADCVal[2] = {0,0};
  for(unsigned curConf=0; curConf<2; curConf++)
    {


      uint16_t SensorConf99 = (0x20 + 1*curConf) * ( Reg==3 || Reg==4) //Tmp/Rad Sensor 1
                            + (0x800 + curConf*0x40) * (Reg==5 || Reg==6); //Tmp/Rad Sensor 2
      uint16_t SensorConf100 = (0x20 + 1*curConf) * ( Reg==14 || Reg==15) //Tmp/Rad Sensor 3
	                     + (0x800 + curConf*0x40) * (Reg==7 || Reg==8);  //Tmp/Rad Sensor 4
      
      if(Reg==3 || Reg==4 || Reg==5 || Reg==6)
	tmpFE->writeRegister(&Rd53a::SensorCfg0, SensorConf99);
      else if(Reg==7 || Reg==8 || Reg==14 || Reg==15)
	tmpFE->writeRegister(&Rd53a::SensorCfg1, SensorConf100);

      tmpFE->idle();

      ADCVal[curConf]=this->ReadADC(Reg,false,dynamic_cast<Rd53a*>(tmpFE));

    }

  return std::pair<uint16_t,uint16_t> (ADCVal[0],ADCVal[1]);
}



void Rd53aReadRegLoop::init() {
    if (verbose)
        std::cout << __PRETTY_FUNCTION__ << std::endl;
    m_done = false;

    //Enables Ring Oscillators to be triggered later
    keeper->globalFe<Rd53a>()->writeRegister(&Rd53a::RingOscEn, m_EnblRingOsc);


    if(m_STDReg.size()==1 && m_STDReg[0]=="All"){
      m_STDReg.clear();
      for (std::pair<std::string, Rd53aReg Rd53aGlobalCfg::*> tmpMap : keeper->globalFe<Rd53a>()->regMap) {
	m_STDReg.push_back(tmpMap.first);

      }
    }

}

void Rd53aReadRegLoop::execPart1() {


  dynamic_cast<HwController*>(g_rx)->setupMode(); //This is need to make sure the global rests doesn't refresh the ADCRegister


  //Currently for RD53A, each board needs to be configured alone due to a bug with the read out of the rd53a. This can be changed in rd53b. 
  for (auto *fe : keeper->feList) {
    if(fe->getActive()) {


      std::cout<<"Measuring for FE: "<<dynamic_cast<FrontEndCfg*>(fe)->getTxChannel()<<std::endl;


      ///--------------------------------///
      //Reading Standard Registers
      for (auto Reg : m_STDReg)	{
	if (keeper->globalFe<Rd53a>()->regMap.find(Reg) != keeper->globalFe<Rd53a>()->regMap.end()) {
	  uint16_t RegisterVal =  (dynamic_cast<Rd53a*>(fe)->*(dynamic_cast<Rd53a*>(fe)->regMap[Reg])).ApplyMask( ReadRegister( keeper->globalFe<Rd53a>()->regMap[Reg] , dynamic_cast<Rd53a*>(fe)) );
	  std::cout<<"REG: "<<Reg<<" Value: "<<RegisterVal<<std::endl;

	  uint16_t StoredVal = (dynamic_cast<Rd53a*>(fe)->*(dynamic_cast<Rd53a*>(fe)->regMap[Reg])).read()  ;
	  


	  if ( StoredVal!=RegisterVal  ){
	    std::cout<<"Warning!!! For Reg: "<<Reg<<", the stored register value ("<<StoredVal<<") doesn't match the one on the chip ("<<RegisterVal<<")."<<std::endl;
	  }


	  //Compare the Register with the stored value, it's a safety mechanism. 






	}
	else
	  std::cout<<"Warning!!! Requested Register "<<Reg<<" not found, please check your runcard"<<std::endl;

      }


      ///--------------------------------///
      //Reading Voltage  ADC 
      for( auto Reg : m_VoltMux) {
	  uint16_t ADCVal = ReadADC(Reg, false, dynamic_cast<Rd53a*>(fe));
	  std::cout<<"MON MUX_V: "<<Reg<<" Value: "<<ADCVal<<" -> "<<dynamic_cast<Rd53a*>(fe)->ADCtoV(ADCVal)<<" V "<<std::endl;
      }

      ///--------------------------------///
      //Reading Temp or Rad sensors from the ADC
      for( auto Reg : m_TempMux){
	std::pair<uint16_t, uint16_t> TempVal = ReadTemp(Reg, dynamic_cast<Rd53a*>(fe));
	std::cout<<"MON MUX_V: "<<Reg<<" Bias 0 "<<TempVal.first<<" -> "<<dynamic_cast<Rd53a*>(fe)->ADCtoV(TempVal.first)<<" V Bias 1 "<<TempVal.second<<" -> "<<dynamic_cast<Rd53a*>(fe)->ADCtoV(TempVal.second)<<" V Diff "<<dynamic_cast<Rd53a*>(fe)->ADCtoV(TempVal.second)-dynamic_cast<Rd53a*>(fe)->ADCtoV(TempVal.first)<<" V"<<std::endl;      
      }
      
      //Reading Current ADC
      for( auto Reg : m_CurMux)	{
	uint16_t ADCVal = ReadADC(Reg, true, dynamic_cast<Rd53a*>(fe));
	std::cout<<"MON MUX_C: "<<Reg<<" Value: "<<ADCVal<<std::endl;
      }
      
    }
  }

  ///--------------------------------///
  ///Running the Ring Oscillators ////



  uint16_t RingValues[8][2]; 
  //Reset Ring Osicilator 
  for(uint16_t tmpCount = 0; m_RstRingOsc && tmpCount<8; tmpCount++) {
    if ( ((m_EnblRingOsc >> tmpCount) % 2) == 1) {    
      keeper->globalFe<Rd53a>()->writeRegister(OscRegisters[tmpCount],0);
    }
  }

  std::cout<<"Starting Ring Osc"<<std::endl;
  
  for (auto *fe : keeper->feList) {
    if(fe->getActive()) {
      for(uint16_t tmpCount = 0; tmpCount<8 ; tmpCount++) { 
	if ( ((m_EnblRingOsc >> tmpCount) % 2) == 1) {
	  RingValues[tmpCount][0]=ReadRegister(OscRegisters[tmpCount],dynamic_cast<Rd53a*>(fe)) & 0xFFF;
	  //std::cout<<"RigBuffer "<<tmpCount<<" For FE "<<dynamic_cast<FrontEndCfg*>(fe)->getTxChannel()<<" At Start: "<<RingValues[tmpCount][0]<<std::endl;
	} 
      }
    }
  }

  keeper->globalFe<Rd53a>()->RunRingOsc(m_RingOscDur);

  for (auto *fe : keeper->feList) {
    if(fe->getActive()) {
      for(uint16_t tmpCount = 0; tmpCount<8; tmpCount++) {    
	if ( ((m_EnblRingOsc >> tmpCount) % 2) == 1) {
	  RingValues[tmpCount][1]=ReadRegister(OscRegisters[tmpCount],dynamic_cast<Rd53a*>(fe)) & 0xFFF ;
	  //std::cout<<"RigBuffer For FE: "<<dynamic_cast<FrontEndCfg*>(fe)->getTxChannel()<<" At End: "<<RingValues[tmpCount][1]<<std::endl;
	} 
      }
    }
  }


  for(uint16_t tmpCount = 0; tmpCount<8; tmpCount++) {    
    if ( ((m_EnblRingOsc >> tmpCount) % 2) == 1) {
      std::cout<<"Ring Buffer: "<<tmpCount<<" Values: "<<RingValues[tmpCount][1]<<" - "<<RingValues[tmpCount][0]<<" = "<<RingValues[tmpCount][1]-RingValues[tmpCount][0];  
      std::cout<<" Frequency: "<<float(RingValues[tmpCount][1]-RingValues[tmpCount][0])<<"/2^"<<m_RingOscDur<<" = "<<float(RingValues[tmpCount][1]-RingValues[tmpCount][0])/ (1<<m_RingOscDur) <<" -> "<< float(RingValues[tmpCount][1]-RingValues[tmpCount][0])/ (1<<m_RingOscDur) /6.5e-3<<" MHz "<<std::endl;
    }
  }

    
  dynamic_cast<HwController*>(g_rx)->runMode(); //This is needed to revert back the setupMode
  
}

void Rd53aReadRegLoop::execPart2() {
  m_done = true;
}

void Rd53aReadRegLoop::end() {
    // Reset to min
}

void Rd53aReadRegLoop::writeConfig(json &config) {

  config["EnblRingOsc"] = m_EnblRingOsc;
  config["RstRingOsc"] = m_RstRingOsc;
  config["RingOscDur"] = m_RingOscDur;
  config["CurMux"] = m_CurMux;
  config["Registers"] = m_STDReg;

  //Just joining back the two STD vectors.
  std::vector<uint16_t> SendBack;
  SendBack.reserve(m_VoltMux.size()+m_TempMux.size());
  SendBack.insert(SendBack.end(),m_VoltMux.begin(),m_VoltMux.end());
  SendBack.insert(SendBack.end(),m_TempMux.begin(),m_TempMux.end());
  config["VoltMux"] = SendBack;
    
}

void Rd53aReadRegLoop::loadConfig(json &config) {

  if (!config["EnblRingOsc"].empty())
    m_EnblRingOsc = config["EnblRingOsc"];
  if (!config["RstRingOsc"].empty())
    m_RstRingOsc = config["RstRingOsc"];
  if (!config["RingOscDur"].empty()) {
    m_RingOscDur = config["RingOscDur"];
    if (m_RingOscDur > 9){
	std::cout<<"Global Max duration is 2^9 = 512 cycles, setting the RingOscDur to Max (9)"<<std::endl;
	m_RingOscDur=9;
      }

  }



    if (!config["VoltMux"].empty())
      for(auto Reg : config["VoltMux"])
	{
	  if( ( int(Reg) >=3 && int(Reg)<=8) || (int(Reg)>=14 && int(Reg)<=15) )
	    m_TempMux.push_back(Reg);
	  else
	    m_VoltMux.push_back(Reg);
	}
    if (!config["CurMux"].empty())
      for(auto Reg : config["CurMux"])
	m_CurMux.push_back(Reg);


    if (!config["Registers"].empty())
      for (auto Reg: config["Registers"]) {
	m_STDReg.push_back(Reg);

	// If Reg is ALL, instead loop over all registers
	if (Reg=="All"){
	  m_STDReg.clear();
	  m_STDReg.push_back(Reg);
	  
	  break;
	}



      }
}





















