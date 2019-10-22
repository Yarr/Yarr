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


std::pair<uint32_t, uint32_t> RegRead(uint32_t higher, uint32_t lower) {
    if ((higher & 0x55000000) == 0x55000000) {
        return std::make_pair((lower>>16)&0x3FF, lower&0xFFFF);
    } else if ((higher & 0x99000000) == 0x99000000) {
        return std::make_pair((higher>>10)&0x3FF, ((lower>>26)&0x3F)+((higher&0x3FF)<<6));
    } else {
        std::cout << "#ERROR# Could not decode reg read!" << std::endl;
        return std::make_pair(999, 666);
    }
    return std::make_pair(999, 666);
}






uint16_t Rd53aReadRegLoop::ReadADC(unsigned short Reg, bool doCur=false) {
      
  keeper->globalFe<Rd53a>()->confADC(Reg,doCur);
  keeper->globalFe<Rd53a>()->readRegister(m_AdcRead);

  std::this_thread::sleep_for(std::chrono::milliseconds(10));

  RawData *data = g_rx->readData();
  unsigned size =  data->words;
        
  for(unsigned c=0; c<size/2; c++)
    {
      if (c*2+1<size) {
	std::pair<uint32_t, uint32_t> readReg = RegRead(data->buf[c*2],data->buf[c*2+1]);	    
	//std::cout<<"MUX: "<<Reg<<" Value: "<<readReg.second<<std::endl;
	return readReg.second;
	
      }
      else {
	std::cout<<"Hallfword recieved"<<data->buf[c*2]<<std::endl;
	return 65535;
      }
    }
    
}

uint16_t Rd53aReadRegLoop::ReadTemp(unsigned short Reg) {

  //Sensor Config
  uint16_t ADCVal[2] = {0,0};
  for(unsigned curConf=0; curConf<2; curConf++)
    {


      uint16_t SensorConf99 = (0x20 + 1*curConf) * ( Reg==3 || Reg==4) //Tmp/Rad Sensor 1
                            + (0x800 + curConf*0x40) * (Reg==5 || Reg==6); //Tmp/Rad Sensor 2
      uint16_t SensorConf100 = (0x20 + 1*curConf) * ( Reg==14 || Reg==15) //Tmp/Rad Sensor 3
	                     + (0x800 + curConf*0x40) * (Reg==7 || Reg==8);  //Tmp/Rad Sensor 4
      
      if(Reg==3 || Reg==4 || Reg==5 || Reg==6)
	keeper->globalFe<Rd53a>()->writeRegister(m_sensorConf99, SensorConf99);
      else if(Reg==7 || Reg==8 || Reg==14 || Reg==15)
	keeper->globalFe<Rd53a>()->writeRegister(m_sensorConf100, SensorConf100);

      //std::this_thread::sleep_for(std::chrono::milliseconds(10));
      keeper->globalFe<Rd53a>()->idle();

      ADCVal[curConf]=this->ReadADC(Reg);

      std::this_thread::sleep_for(std::chrono::milliseconds(10));

    }
  std::cout<<"MUX: "<<Reg<<" Bias 0 "<<ADCVal[0]<<" Bias 1 "<<ADCVal[1]<<" Diff "<<ADCVal[1]-ADCVal[0]<<std::endl;
  return ADCVal[1]-ADCVal[0];
}



void Rd53aReadRegLoop::init() {
    if (verbose)
        std::cout << __PRETTY_FUNCTION__ << std::endl;
    m_done = false;


    m_AdcRead = keeper->globalFe<Rd53a>()->regMap["AdcRead"];
    m_sensorConf99 = keeper->globalFe<Rd53a>()->regMap["SensorCfg0"];
    m_sensorConf100 = keeper->globalFe<Rd53a>()->regMap["SensorCfg1"];


}

void Rd53aReadRegLoop::execPart1() {


  for( auto Reg : VoltMux)
    {
      uint16_t ADCVal = ReadADC(Reg);
      std::cout<<"MUX: "<<Reg<<" Value: "<<ADCVal<<std::endl;
    }
  std::this_thread::sleep_for(std::chrono::milliseconds(10));

  for( auto Reg : TempMux){
    ReadTemp(Reg);
  }

  for( auto Reg : CurMux)
    {
      uint16_t ADCVal = ReadADC(Reg, true);
      std::cout<<"CURMUX: "<<Reg<<" Value: "<<ADCVal<<std::endl;
    }
  std::this_thread::sleep_for(std::chrono::milliseconds(10));



  
}

void Rd53aReadRegLoop::execPart2() {
  m_done = true;
}

void Rd53aReadRegLoop::end() {
    // Reset to min
}





void Rd53aReadRegLoop::writeConfig(json &config) {
  //config["MUXREG"] = TempMux+VoltMux;
    
}

void Rd53aReadRegLoop::loadConfig(json &config) {


    if (!config["VOLTMUX"].empty())
      for(auto Reg : config["VOLTMUX"])
	{
	  if( ( int(Reg) >=3 && int(Reg)<=8) || (int(Reg)>=14 && int(Reg)<=15) )
	    TempMux.push_back(Reg);
	  else
	    VoltMux.push_back(Reg);
	}
    if (!config["CURMUX"].empty())
      for(auto Reg : config["CURMUX"])
	CurMux.push_back(Reg);
}





















