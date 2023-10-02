#ifndef STAR_CFG_INCLUDE
#define STAR_CFG_INCLUDE

// #################################
// # Project:
// # Description: StarChips Library
// # Comment: Star configuration class
// ################################

#include <algorithm>
#include <cmath>
#include <functional>
#include <tuple>
#include <iostream>

#include "FrontEnd.h"

#include "AbcCfg.h"
#include "HccCfg.h"

#include "StarConversionTools.h"

/// Represents configuration for one particular Star front-end (HCC + ABCs)
class StarCfg : public FrontEndCfg {
 public:
  StarCfg(int abc_version, int hcc_version);
  ~StarCfg() override;

  //Function to make all Registers for the ABC
  void configure_ABC_Registers(int chipID);

  //Accessor functions
  uint32_t getHCCRegister(HCCStarRegister addr);
  void     setHCCRegister(HCCStarRegister addr, uint32_t val);
  uint32_t getABCRegister(ABCStarRegister addr, int32_t chipID );
  void     setABCRegister(ABCStarRegister addr, uint32_t val, int32_t chipID);
  // Overload with integer register address
  inline const uint32_t getHCCRegister(uint32_t addr) {
    return getHCCRegister(HCCStarRegister::_from_integral(addr));
  }
  inline void setHCCRegister(uint32_t addr, uint32_t val) {
    setHCCRegister(HCCStarRegister::_from_integral(addr), val);
  }
  inline const uint32_t getABCRegister(uint32_t addr, int32_t chipID ) {
    return getABCRegister(ABCStarRegister(ABCStarRegs::_from_integral(addr)), chipID);
  }
  inline void setABCRegister(uint32_t addr, uint32_t val, int32_t chipID) {
    setABCRegister(ABCStarRegister(ABCStarRegs::_from_integral(addr)), val, chipID);
  }

  unsigned int getHCCchipID(){ return m_hcc.getHCCchipID(); }
  void setHCCChipId(unsigned hccID){ m_hcc.setHCCChipId(hccID); }

  const unsigned int getABCchipID(unsigned int chipIndex) { return abcFromIndex(chipIndex).getABCchipID(); }

  void addABCchipID(unsigned int chipID) {
      if (m_ABCchips.size() == 0)
          addABCchipID(chipID, 0);
      //highest key +1 (~std::vector::push_back)
      else
          addABCchipID(chipID, m_ABCchips.rbegin()->first +1);
  }

  void addABCchipID(unsigned int chipID, unsigned int hccIn) {
      m_ABCchips.emplace(hccIn, m_abc_version);
      m_ABCchips.at(hccIn).setABCChipId(chipID);
  }

  void clearABCchipIDs() { m_ABCchips.clear();}

  void setSubRegisterValue(int chipIndex, std::string subRegName, uint32_t value) {
    if (!chipIndex && HCCStarSubRegister::_is_valid(subRegName.c_str())) { //If HCC, looking name
      return m_hcc.setSubRegisterValue(subRegName, value);
    } else if (chipIndex && ABCStarSubRegister::_is_valid(subRegName.c_str())) { //If looking for an ABC subregister enum
        if (abcAtIndex(chipIndex))
            return abcFromIndex(chipIndex).setSubRegisterValue(subRegName, value);
    }else {
      std::cerr << " --> Error: Could not find register \""<< subRegName << "\"" << std::endl;
    }
  }


  uint32_t getSubRegisterValue(int chipIndex, std::string subRegName) {
    if (!chipIndex && HCCStarSubRegister::_is_valid(subRegName.c_str())) { //If HCC, looking name
      return m_hcc.getSubRegisterValue(subRegName);
    } else if (chipIndex && ABCStarSubRegister::_is_valid(subRegName.c_str())) { //If looking for an ABC subregister enum
        if (abcAtIndex(chipIndex))
            return abcFromIndex(chipIndex).getSubRegisterValue(subRegName);
    }else {
      std::cerr << " --> Error: Could not find register \""<< subRegName << "\"" << std::endl;
    }
    return 0;
  }

  int getSubRegisterParentAddr(int chipIndex, std::string subRegName) {
    if (!chipIndex && HCCStarSubRegister::_is_valid(subRegName.c_str())) { //If HCC, looking name
      return m_hcc.getSubRegisterParentAddr(subRegName);
    } else if (chipIndex && ABCStarSubRegister::_is_valid(subRegName.c_str())) { //If looking for an ABC subregister enum
      return m_abc_info->getSubRegisterParentAddr(subRegName);
    }else {
      std::cerr << " --> Error: Could not find register \""<< subRegName << "\"" << std::endl;
    }
    return 0;
  }


  uint32_t getSubRegisterParentValue(int chipIndex, std::string subRegName) {
    if (!chipIndex && HCCStarSubRegister::_is_valid(subRegName.c_str())) { //If HCC, looking name
      return m_hcc.getSubRegisterParentValue(subRegName);
    } else if (chipIndex && ABCStarSubRegister::_is_valid(subRegName.c_str())) { //If looking for an ABC subregister enum
        if (abcAtIndex(chipIndex))
            return abcFromIndex(chipIndex).getSubRegisterParentValue(subRegName);
    }else {
      std::cerr << " --> Error: Could not find register \""<< subRegName << "\"" << std::endl;
    }
    return 0;
  }

  void maskPixel(unsigned col, unsigned row) override {}
  unsigned getPixelEn(unsigned col, unsigned row) override {
    return 1; // getPixelEn() was desgined for Pixels, further modification is needed for StarChip
  }
  void enableAll() override;

  /**
   * Obtain the corresponding charge [e] from the input VCal
   */
  double toCharge(double vcal) override;

  /**
   * Obtain the corresponding charge [e] from the input VCal, small&large capacitances(?)
   * Not fully implmented yet.
   */
  double toCharge(double vcal, bool sCap, bool lCap) override;

  /// Set trim DAC based on col/row in histogram
  void setTrimDAC(unsigned col, unsigned row, int value);

  /// Get trim DAC based on col/row in histogram
  int getTrimDAC(unsigned col, unsigned row) const;


  void writeConfig(json &j) override;
  void loadConfig(const json &j) override;

  using configFuncMap = std::unordered_map<std::string, std::tuple<json, std::vector<json>>(StarCfg::*)(void)>;
  static configFuncMap createConfigs;
  std::tuple<json, std::vector<json>> getPreset(const std::string& systemType) override;

  size_t numABCs() { return m_ABCchips.size(); }
  int highestABC() { 
      if (m_ABCchips.size() == 0)
          return -1;
      return m_ABCchips.rbegin()->first; 
  } 
  int lowestABC() { 
      if (m_ABCchips.size() == 0)
          return -1;
      return m_ABCchips.begin()->first; 
  } 

  /// Iterate over ABCs, avoiding chipIndex
  void eachAbc(std::function<void (AbcCfg&)> f) {
    for(auto &abc: m_ABCchips) {
        f(abc.second);
    }
  }

  HccCfg &hcc() { return m_hcc; }

  int hccChannelForABCchipID(unsigned int chipID);

  StarConversionTools &getStarConversion() {return m_ct;}

  int abcVersion() const {return m_abc_version;}
  int hccVersion() const {return m_hcc_version;}

 protected:
  AbcCfg &abcFromChipID(unsigned int chipID) {
      for(auto &abcPair : m_ABCchips) {
          auto &abc = abcPair.second;
          if (abc.getABCchipID() == chipID)
              return abc;
      }
      return (*m_ABCchips.end()).second;
  }

  std::shared_ptr<const AbcStarRegInfo> m_abc_info; 
  std::shared_ptr<const HccStarRegInfo> m_hcc_info; 

  uint32_t m_sn=0;//serial number set by eFuse bits

  int m_abc_version; 
  int m_hcc_version; 

  HccCfg m_hcc;

  std::map<unsigned, AbcCfg> m_ABCchips;

  bool abcAtIndex(int chipIndex) const {
    assert(chipIndex > 0);
    return (m_ABCchips.count(chipIndex-1) > 0);
  }

  AbcCfg &abcFromIndex(int chipIndex) {
    assert(abcAtIndex(chipIndex));
    return m_ABCchips.at(chipIndex-1);
  }

  const AbcCfg &abcFromIndex(int chipIndex) const {
    assert(chipIndex > 0);
    assert(abcAtIndex(chipIndex));
    return m_ABCchips.at(chipIndex-1);
  }

  std::tuple<json, std::vector<json>> createConfigSingleFE();
  std::tuple<json, std::vector<json>> createConfigLSStave();
  std::tuple<json, std::vector<json>> createConfigPetal();

  StarConversionTools m_ct;
};

#endif
