#ifndef STAR_CHIPS_HEADER_
#define STAR_CHIPS_HEADER_

// #################################
// # Project:
// # Description: StarChips Library
// # Comment: StarChips FrontEnd class
// ################################

#include <string>

#include "FrontEnd.h"

class TxCore;
class RxCore;

#include "StarCmd.h"
#include "StarCfg.h"

class StarChips : public StarCfg, public StarCmd, public FrontEnd {
 public:
    StarChips();
    StarChips(HwController *arg_core);
    StarChips(HwController *arg_core, unsigned arg_channel);
    StarChips(HwController *arg_core, unsigned arg_txchannel, unsigned arg_rxchannel);

    ~StarChips() {}

    void init(HwController *arg_core, unsigned arg_txChannel, unsigned arg_rxChannel) override;

    void writeNamedRegister(std::string name, uint16_t value) override;

    // Pixel specific?
    void setInjCharge(double, bool, bool) override {}
    void maskPixel(unsigned col, unsigned row) override {}

    //! configure
    //! brief configure the chip (virtual)
    void configure() override final;

    //! toFileJson
    //! brief write configuration to json (virtual)
    //! param reference to json
    void toFileJson(json&) override;

    //! fromFileJson
    //! brief read configuration from json (virtual)
    //! param reference to json
    void fromFileJson(json&) override;

    void makeGlobal() override final {
    	m_hccID = 15;
    }

    void reset();
    void sendCmd(std::array<uint16_t, 9> cmd);
    void sendCmd(uint16_t cmd);

    bool writeRegisters();
    void readRegisters();


    uint32_t getRegister(uint32_t addr, int32_t chipID = 0 );
    void setRegister(uint32_t addr, uint32_t val, int32_t chipID = 0);


    void setAndWriteRegister(int addr, int64_t  value=-1, int32_t chipID = 0){
  	  if(value>0){
  		std::cout << value << std::endl;
  		  registerMap[chipID][addr]->setValue((uint32_t) value);
  	  }
  	  if( chipID == 0 ){
  		  if(m_debug)  std::cout << "Doing HCC setAndWriteRegister with value 0x" << std::hex << std::setfill('0') << std::setw(8) << registerMap[chipID][addr]->getValue() <<std::dec  << std::endl;
  		  sendCmd(write_hcc_register(addr, registerMap[chipID][addr]->getValue(), m_hccID));
  	  }else{
  		  if(m_debug)  std::cout << "Doing ABC " << chipID << " setAndWriteRegister with value 0x" << std::hex << std::setfill('0') << std::setw(8) << registerMap[chipID][addr]->getValue() <<std::dec  << std::endl;
  		  sendCmd(write_abc_register(addr, registerMap[chipID][addr]->getValue(), m_hccID, chipID));

  	  }
    }


    void readRegister(int addr, int32_t chipID = 0){
    	if( chipID == 0 )sendCmd(read_hcc_register(addr, m_hccID));
    	else sendCmd(read_abc_register(addr, 0xf, chipID));
    }


    void setAndWriteSubRegister(std::string subRegName, uint32_t value, int32_t chipID = 0){
  	  setSubRegisterValue(chipID, subRegName,value);

  	  if( chipID == 0 ) sendCmd( write_hcc_register(getSubRegisterParentAddr(chipID, subRegName), getSubRegisterParentValue(chipID, subRegName), m_hccID) );
  	  else  sendCmd( write_abc_register(getSubRegisterParentAddr(chipID, subRegName), getSubRegisterParentValue(chipID, subRegName), m_hccID, chipID) );

    }


    void readSubRegister(std::string subRegName, int32_t chipID = 0){
    	if( chipID == 0 )sendCmd(read_hcc_register(getSubRegisterParentAddr(chipID, subRegName), m_hccID));
    	else sendCmd(read_abc_register(getSubRegisterParentAddr(chipID, subRegName), 0xf, chipID));
  	    }


    const int getNumberOfAssociatedABC(){return m_nABC;}

    int m_nABC = 0;
    std::vector<int> m_chipIDs;




  private:
    TxCore * m_txcore;
};

#endif
