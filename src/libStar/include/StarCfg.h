#ifndef STAR_CFG_INCLUDE
#define STAR_CFG_INCLUDE

// #################################
// # Project:
// # Description: StarChips Library
// # Comment: Star configuration class
// ################################

#include <algorithm>
#include <iostream>
#include <iomanip>
#include <fstream>
#include <cmath>
#include "BitOps.h"

#include "FrontEnd.h"
#include "storage.hpp"




class SubRegister{

 public:

  SubRegister(){
    m_parentReg			= 0;
    m_parentRegAddress	= -1;
    m_subRegName			= "";
    m_bOffset 			= 0;
    m_mask				= 0;
    m_msbRight 			= false;
    m_value 				= 0;
  };


  SubRegister(uint32_t *reg=NULL, int parentRegAddress=-1, std::string subRegName="", unsigned bOffset=0, unsigned mask=0, bool msbRight=false){
    m_parentReg			= reg;
    m_parentRegAddress	= parentRegAddress;
    m_subRegName			= subRegName;
    m_bOffset 			= bOffset;
    m_mask				= mask;
    m_msbRight			= msbRight;

  };


  // Get value of field
  const unsigned getValue() {
    unsigned maskBits = (1<<m_mask)-1;
    unsigned tmp = ((*m_parentReg&(maskBits<<m_bOffset))>>m_bOffset);

    if(m_msbRight?BitOps::reverse_bits(tmp, m_mask):tmp != m_value){
      std::cerr << " --> Error: Stored value and retrieve value does not match: \""<< m_subRegName << "\"" << std::endl;
    }

    return (m_msbRight?BitOps::reverse_bits(tmp, m_mask):tmp);
  }



  // Write value to field and config
  void updateValue(const uint32_t cfgBits) {
    m_value = cfgBits;

    unsigned maskBits = (1<<m_mask)-1;
    *m_parentReg=(*m_parentReg&(~(maskBits<<m_bOffset))) |
      (((m_msbRight?BitOps::reverse_bits(cfgBits, m_mask):cfgBits)&maskBits)<<m_bOffset);
  }


  std::string name() const{ return m_subRegName; }


  int getParentRegAddress() const{ return m_parentRegAddress; }

  uint32_t getParentRegValue() const{ return *m_parentReg;	}



 private:
  uint32_t *m_parentReg;
  int m_parentRegAddress;
  std::string m_subRegName;
  unsigned m_bOffset;
  unsigned m_mask;
  bool m_msbRight;
  unsigned m_value;



};



class Register{

 public:

  std::map<std::string, SubRegister*> subRegisterMap;

  Register(int addr=-1, uint32_t value=0){
    m_regAddress		= addr;
    m_regValue			= value;
  };


  ~Register(){
    std::map<std::string, SubRegister*>::iterator map_iter;
    for(map_iter=subRegisterMap.begin(); map_iter!= subRegisterMap.end(); ++map_iter){
      delete map_iter->second;
    }
    subRegisterMap.clear();
  };




  int addr() const{ return m_regAddress;}
  const uint32_t getValue() const{ return m_regValue;}
  void setValue(uint32_t value) {m_regValue	= value;}
  void setMySubRegisterValue(std::string subRegName, uint32_t value){
    subRegisterMap[subRegName]->updateValue(value);
  }

  const unsigned getMySubRegisterValue(std::string subRegName){
    return subRegisterMap[subRegName]->getValue();
  }


  SubRegister* addSubRegister(std::string subRegName="", unsigned bOffset=0, unsigned mask=0, bool msbRight=false){
    subRegisterMap[subRegName] = new SubRegister(&m_regValue, m_regAddress, subRegName,bOffset, mask, msbRight);
    return subRegisterMap[subRegName];
  }


 private:
  int m_regAddress;
  uint32_t m_regValue;



};




class StarCfg : public FrontEndCfg, public Register{
 public:
  StarCfg();
  ~StarCfg();


  //Function to make all Registers for the HCC and ABC
  void configure_HCC_Registers();
  void configure_ABC_Registers(int chipID);

  //Accessor functions
  const uint32_t getHCCRegister(uint32_t addr);
  void     setHCCRegister(uint32_t addr, uint32_t val);
  const uint32_t getABCRegister(uint32_t addr, int32_t chipID = 0 );
  void     setABCRegister(uint32_t addr, uint32_t val, int32_t chipID = 0);


  //Initialized the registers of the HCC and ABC.  Do afer JSON file is loaded.
  void initRegisterMaps();

  const unsigned int getHCCchipID(){return m_hccID;}
  void setHCCChipId(unsigned hccID){
    m_hccID = hccID;
  }

  const unsigned int getABCchipID(unsigned int chipIndex) { return m_ABCchipIDs[chipIndex-1];};



  void setSubRegisterValue(int chipIndex, std::string subRegName, uint32_t value) {
    if (subRegisterMap_all[chipIndex].find(subRegName) != subRegisterMap_all[chipIndex].end()) {
      subRegisterMap_all[chipIndex][subRegName]->updateValue(value);
    } else {
      std::cerr << " --> Error: Could not find register \""<< subRegName << "\"" << std::endl;
    }
  }


  uint32_t getSubRegisterValue(int chipIndex, std::string subRegName) {
    if (subRegisterMap_all[chipIndex].find(subRegName) != subRegisterMap_all[chipIndex].end()) {

      return subRegisterMap_all[chipIndex][subRegName]->getValue();
    } else {
      std::cerr << " --> Error: Could not find register \""<< subRegName << "\"" << std::endl;
    }
    return 0;
  }


  int getSubRegisterParentAddr(int chipIndex, std::string subRegName) {
    if (subRegisterMap_all[chipIndex].find(subRegName) != subRegisterMap_all[chipIndex].end()) {
      return subRegisterMap_all[chipIndex][subRegName]->getParentRegAddress();
    } else {
      std::cerr << " --> Error: Could not find register \""<< subRegName << "\"" << std::endl;
    }
    return 0;
  }


  uint32_t getSubRegisterParentValue(int chipIndex, std::string subRegName) {
    if (subRegisterMap_all[chipIndex].find(subRegName) != subRegisterMap_all[chipIndex].end()) {
      return subRegisterMap_all[chipIndex][subRegName]->getParentRegValue();
    } else {
      std::cerr << " --> Error: Could not find register \""<< subRegName << "\"" << std::endl;
    }
    return 0;
  }

  /**
   * Obtain the corresponding charge [e] from the input VCal
   */
  double toCharge(double vcal);

  /**
   * Obtain the corresponding charge [e] from the input VCal, small&large capacitances(?)
   * Not fully implmented yet.
   */
  double toCharge(double vcal, bool sCap, bool lCap);

  /**
   * Obtain the corresponding VCal from the input charge [e]
   */
  unsigned toVcal(double charge);


  void setTrimDAC(unsigned col, unsigned row, int value);

  int getTrimDAC(unsigned col, unsigned row);


  void toFileJson(json &j) override;
  void fromFileJson(json &j) override;

  void toFileBinary() override;
  void fromFileBinary() override;

  int m_nABC = 0;


 protected:
  const unsigned int indexForABCchipID(unsigned int chipID) {return std::distance(m_ABCchipIDs.begin(), std::find(m_ABCchipIDs.begin(), m_ABCchipIDs.end(), chipID)) + 1;};
  void addABCchipID(unsigned int chipID) { m_ABCchipIDs.push_back(chipID);};
  void clearABCchipIDs() { m_ABCchipIDs.clear(); };
    
  unsigned m_hccID;

  //This saves all Register objects to memory, purely for storage.  We will never access registers from this
  std::vector< Register > AllReg_List;
  //This is a 2D map of each register to the chip index (0 for HCC, iABC+1 for ABCs) and address.  For example registerMap[chip index][addr]
  std::map<unsigned, std::map<unsigned, Register*> >registerMap; //Maps register address
  //This is a 2D map of each subregister to the chip index and subregister name.  For example subRegisterMap_all[chip index][NAME]
  std::map<unsigned, std::map<std::string, SubRegister*> > subRegisterMap_all;   //register record


  //This is a 2D map of each trimDac_32b register to the chip index and trimDAC_4LSB register name.  For example trimDAC4LSB_RegisterMap_all[chip index][NAME]
  std::map<unsigned, std::map<std::string, SubRegister*> > trimDAC_4LSB_RegisterMap_all;   //register record

  //This is a 2D map of each trimDac_32b register to the chip index and trimDAC_1MSB register name.  For example trimDAC1LSB_RegisterMap_all[chip index][NAME]
  std::map<unsigned, std::map<std::string, SubRegister*> > trimDAC_1MSB_RegisterMap_all;   //register record


 private:
  std::vector<unsigned int> m_ABCchipIDs;
  float m_injCap; //fF
  std::array<float, 4> m_vcalPar; //mV, [0] + [1]*x + [2]*x^2 + [3]*x^3

};

#endif
