#ifndef STAR_CFG_INCLUDE
#define STAR_CFG_INCLUDE

// #################################
// # Project:
// # Description: StarChips Library
// # Comment: Star configuration class
// ################################

#include <algorithm>
#include <cmath>
#include <tuple>

#include "FrontEnd.h"
#include "storage.hpp"

#include "AbcCfg.h"
#include "HccCfg.h"

/// Lookup information on Star register map
class StarRegInfo {
  public:
  /// Fills the maps appropriately
  StarRegInfo();

  //This is a map from each register address to the register info.  Thus abcregisterMap[addr]
  std::map<unsigned, std::shared_ptr<RegisterInfo>> abcregisterMap;
  //This is a map from each register address to the register info.  Thus hccregisterMap[addr]
  std::map<unsigned, std::shared_ptr<RegisterInfo>> hccregisterMap;

  //This is a 2D map of each subregister to the HCC subregister name.  For example hccSubRegisterMap_all[NAME]
  // SubRegister is owned by the Registers
  std::map<HCCStarSubRegister, std::shared_ptr<SubRegisterInfo>> hccSubRegisterMap_all;   //register record
  //This is a 2D map of each subregister to the ABC subregister name.  For example abcSubRegisterMap_all[NAME]
  std::map<ABCStarSubRegister, std::shared_ptr<SubRegisterInfo>> abcSubRegisterMap_all;

  //This is a 2D map of each trimDac_32b register to the chip index and trimDAC_4LSB register name.  For example trimDAC4LSB_RegisterMap_all[chip index][NAME]
  std::map<int, std::shared_ptr<SubRegisterInfo>> trimDAC_4LSB_RegisterMap_all;

  //This is a 2D map of each trimDac_32b register to the chip index and trimDAC_1MSB register name.  For example trimDAC1LSB_RegisterMap_all[chip index][NAME]
  std::map<int, std::shared_ptr<SubRegisterInfo>> trimDAC_1MSB_RegisterMap_all;

  static std::shared_ptr<StarRegInfo> instance() {
    if(!m_instance) m_instance.reset(new StarRegInfo);
    return m_instance;
  }
 private:
  static std::shared_ptr<StarRegInfo> m_instance;
};

/// Represents configuration for one particular Star front-end (HCC + ABCs)
class StarCfg : public FrontEndCfg {
 public:
  StarCfg();
  ~StarCfg();


  //Function to make all Registers for the HCC and ABC
  void configure_HCC_Registers();
  void configure_ABC_Registers(int chipID);

  //Accessor functions
  const uint32_t getHCCRegister(uint32_t addr);
  void     setHCCRegister(uint32_t addr, uint32_t val);
  const uint32_t getABCRegister(uint32_t addr, int32_t chipID );
  void     setABCRegister(uint32_t addr, uint32_t val, int32_t chipID);


  //Initialized the registers of the HCC and ABC.  Do afer JSON file is loaded.
  void initRegisterMaps();

  const unsigned int getHCCchipID(){return m_hccID;}
  void setHCCChipId(unsigned hccID){
    m_hccID = hccID;
  }

  const unsigned int getABCchipID(unsigned int chipIndex) { return m_ABCchipIDs[chipIndex-1];};

  void addABCchipID(unsigned int chipID) { m_ABCchipIDs.push_back(chipID); };
  void clearABCchipIDs() { m_ABCchipIDs.clear(); };

  void setSubRegisterValue(int chipIndex, std::string subRegName, uint32_t value) {
    if (!chipIndex && HCCStarSubRegister::_is_valid(subRegName.c_str())) { //If HCC, looking name
      auto info = m_info->hccSubRegisterMap_all[HCCStarSubRegister::_from_string(subRegName.c_str())];
      registerMap[chipIndex][info->m_regAddress]->getSubRegister(info).updateValue(value);
    } else if (chipIndex && ABCStarSubRegister::_is_valid(subRegName.c_str())) { //If looking for an ABC subregister enum
      auto info = m_info->abcSubRegisterMap_all[ABCStarSubRegister::_from_string(subRegName.c_str())];
      registerMap[chipIndex][info->m_regAddress]->getSubRegister(info).updateValue(value);
    }else {
      std::cerr << " --> Error: Could not find register \""<< subRegName << "\"" << std::endl;
    }
  }


  uint32_t getSubRegisterValue(int chipIndex, std::string subRegName) {
    if (!chipIndex && HCCStarSubRegister::_is_valid(subRegName.c_str())) { //If HCC, looking name
      auto info = m_info->hccSubRegisterMap_all[HCCStarSubRegister::_from_string(subRegName.c_str())];
      return registerMap[chipIndex][info->m_regAddress]->getSubRegister(info).getValue();
    } else if (chipIndex && ABCStarSubRegister::_is_valid(subRegName.c_str())) { //If looking for an ABC subregister enum
      auto info = m_info->abcSubRegisterMap_all[ABCStarSubRegister::_from_string(subRegName.c_str())];
      return registerMap[chipIndex][info->m_regAddress]->getSubRegister(info).getValue();
    }else {
      std::cerr << " --> Error: Could not find register \""<< subRegName << "\"" << std::endl;
    }
    return 0;
  }


  int getSubRegisterParentAddr(int chipIndex, std::string subRegName) {
    if (!chipIndex && HCCStarSubRegister::_is_valid(subRegName.c_str())) { //If HCC, looking name
      auto info = m_info->hccSubRegisterMap_all[HCCStarSubRegister::_from_string(subRegName.c_str())];
      return info->getRegAddress();
    } else if (chipIndex && ABCStarSubRegister::_is_valid(subRegName.c_str())) { //If looking for an ABC subregister enum
      auto info = m_info->abcSubRegisterMap_all[ABCStarSubRegister::_from_string(subRegName.c_str())];
      return info->getRegAddress();
    }else {
      std::cerr << " --> Error: Could not find register \""<< subRegName << "\"" << std::endl;
    }
    return 0;
  }


  uint32_t getSubRegisterParentValue(int chipIndex, std::string subRegName) {
    if (!chipIndex && HCCStarSubRegister::_is_valid(subRegName.c_str())) { //If HCC, looking name
      auto info = m_info->hccSubRegisterMap_all[HCCStarSubRegister::_from_string(subRegName.c_str())];
      return registerMap[chipIndex][info->m_regAddress]->getValue();
    } else if (chipIndex && ABCStarSubRegister::_is_valid(subRegName.c_str())) { //If looking for an ABC subregister enum
      auto info = m_info->abcSubRegisterMap_all[ABCStarSubRegister::_from_string(subRegName.c_str())];
      return registerMap[chipIndex][info->m_regAddress]->getValue();
    }else {
      std::cerr << " --> Error: Could not find register \""<< subRegName << "\"" << std::endl;
    }
    return 0;
  }

  /**
   * Obtain the corresponding charge [e] from the input VCal
   */
  double toCharge(double vcal) override;

  /**
   * Obtain the corresponding charge [e] from the input VCal, small&large capacitances(?)
   * Not fully implmented yet.
   */
  double toCharge(double vcal, bool sCap, bool lCap) override;

  void setTrimDAC(unsigned col, unsigned row, int value);

  int getTrimDAC(unsigned col, unsigned row);


  void toFileJson(json &j) override;
  void fromFileJson(json &j) override;

 protected:
  const unsigned int indexForABCchipID(unsigned int chipID) {return std::distance(m_ABCchipIDs.begin(), std::find(m_ABCchipIDs.begin(), m_ABCchipIDs.end(), chipID)) + 1;};
    
  unsigned m_hccID;

  //This saves all Register objects to memory, purely for storage.  We will never access registers from this
  std::vector< Register > AllReg_List;
//This is a 2D map of each register to the chip index (0 for HCC, iABC+1 for ABCs) and address.  For example registerMap[chip index][addr]
  std::map<unsigned, std::map<unsigned, Register*> >registerMap; //Maps register address

  size_t numABCs() { return m_ABCchipIDs.size(); }

 private:
  std::shared_ptr<StarRegInfo> m_info;

  std::vector<unsigned int> m_ABCchipIDs;

  SubRegister getAbcSubRegister(int chipIndex, ABCStarSubRegister r) {
    auto info = m_info->abcSubRegisterMap_all[r];
    return registerMap[chipIndex][info->m_regAddress]->getSubRegister(info);
  }

  SubRegister getHccSubRegister(HCCStarSubRegister r) {
    auto info = m_info->hccSubRegisterMap_all[r];
    return registerMap[0][info->m_regAddress]->getSubRegister(info);
  }
};

#endif
