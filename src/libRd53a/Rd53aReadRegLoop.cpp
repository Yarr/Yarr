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



}

void Rd53aReadRegLoop::execPart1() {


  dynamic_cast<HwController*>(g_rx)->setupMode(); //This is need to make sure the global rests doesn't refresh the ADCRegister


  //Currently for RD53A, each board needs to be configured alone due to a bug with the read out of the rd53a. This can be changed in rd53b. 
  for (auto *fe : keeper->feList) {
    if(fe->getActive()) {


      std::cout<<"Measuring for FE: "<<dynamic_cast<FrontEndCfg*>(fe)->getTxChannel()<<std::endl;

      //Reading Standard Registers
      for (auto Reg : m_STDReg)	{
	if (keeper->globalFe<Rd53a>()->regMap.find(Reg) != keeper->globalFe<Rd53a>()->regMap.end()) {
	  uint16_t RegisterVal = ReadRegister( keeper->globalFe<Rd53a>()->regMap[Reg] , dynamic_cast<Rd53a*>(fe));
	  std::cout<<"REG: "<<Reg<<" Value: "<<RegisterVal<<std::endl;
	}
	else
	  std::cout<<"Warning!!! Requested Register "<<Reg<<" not found, please check your runcard"<<std::endl;

      }

      //Reading Voltage  ADC 
      for( auto Reg : m_VoltMux) {
	  uint16_t ADCVal = ReadADC(Reg, false, dynamic_cast<Rd53a*>(fe));
	  std::cout<<"MON MUX_V: "<<Reg<<" Value: "<<ADCVal<<std::endl;
      }
      
      //Reading Temp or Rad sensors from the ADC
      for( auto Reg : m_TempMux){
	std::pair<uint16_t, uint16_t> TempVal = ReadTemp(Reg, dynamic_cast<Rd53a*>(fe));
	std::cout<<"MON MUX_V: "<<Reg<<" Bias 0 "<<TempVal.first<<" Bias 1 "<<TempVal.second<<" Diff "<<TempVal.second-TempVal.first<<std::endl;      
      }
      
      //Reading Current ADC
      for( auto Reg : m_CurMux)	{
	uint16_t ADCVal = ReadADC(Reg, true, dynamic_cast<Rd53a*>(fe));
	std::cout<<"MON MUX_C: "<<Reg<<" Value: "<<ADCVal<<std::endl;
      }
      
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
  config["CURMUX"] = m_CurMux;
  config["REGISTERS"] = m_STDReg;


  //Just joining back the two STD vectors.
  std::vector<uint16_t> SendBack;
  SendBack.reserve(m_VoltMux.size()+m_TempMux.size());
  SendBack.insert(SendBack.end(),m_VoltMux.begin(),m_VoltMux.end());
  SendBack.insert(SendBack.end(),m_TempMux.begin(),m_TempMux.end());
  config["VOLTMUX"] = SendBack;
    
}

void Rd53aReadRegLoop::loadConfig(json &config) {


    if (!config["VOLTMUX"].empty())
      for(auto Reg : config["VOLTMUX"])
	{
	  if( ( int(Reg) >=3 && int(Reg)<=8) || (int(Reg)>=14 && int(Reg)<=15) )
	    m_TempMux.push_back(Reg);
	  else
	    m_VoltMux.push_back(Reg);
	}
    if (!config["CURMUX"].empty())
      for(auto Reg : config["CURMUX"])
	m_CurMux.push_back(Reg);


    if (!config["REGISTERS"].empty())
      for (auto Reg: config["REGISTERS"])
	m_STDReg.push_back(Reg);

}





















