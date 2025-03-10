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

  /// Return value of HCC register
  uint32_t getHCCRegister(HCCStarRegister addr) const;

  /// Set value of HCC register
  void     setHCCRegister(HCCStarRegister addr, uint32_t val);

  /// Get value of ABC register (by ABC communications ID)
  uint32_t getABCRegister(ABCStarRegister addr, int32_t chipID );

  /// Set value of ABC register (by ABC communications ID)
  void     setABCRegister(ABCStarRegister addr, uint32_t val, int32_t chipID);

  /// Return value of HCC register (integer version)
  inline const uint32_t getHCCRegister(uint32_t addr) const {
    return getHCCRegister(HCCStarRegister::_from_integral(addr));
  }

  /// Set value of HCC register (integer version)
  inline void setHCCRegister(uint32_t addr, uint32_t val) {
    setHCCRegister(HCCStarRegister::_from_integral(addr), val);
  }

  /// Get value of ABC register (by integer address and ABC communications ID)
  inline const uint32_t getABCRegister(uint32_t addr, int32_t chipID )
  {
    return getABCRegister((ABCStarRegister)addr, chipID);
  }

  /// Set value of ABC register (by integer address and ABC communications ID)
  inline void setABCRegister(uint32_t addr, uint32_t val, int32_t chipID)
  {
    setABCRegister((ABCStarRegister)addr, val, chipID);
  }

  /// Get the ID used to communicate with this HCC
  unsigned int getHCCchipID(){ return m_hcc.getHCCchipID(); }
  /// Set the ID used to communicate with this HCC
  void setHCCChipId(unsigned hccID){ m_hcc.setHCCChipId(hccID); }

  /// Get the recorded HCC fuse ID (used to set communication ID)
  const uint32_t getHCCfuseID() const{return m_fuse_id;}

  /// Set the HCC fuse ID used to set communication ID
  void setHCCfuseID(uint32_t fuseID) { m_fuse_id = fuseID; }

  /// Get the ID associated with an ABC
  const unsigned int getABCchipID(unsigned int chipIndex) { return abcFromIndex(chipIndex).getABCchipID(); }

  /// Add new ABC with given communications ID
  void addABCchipID(unsigned int chipID) {
      if (m_ABCchips.size() == 0)
          addABCchipID(chipID, 0);
      //highest key +1 (~std::vector::push_back)
      else
          addABCchipID(chipID, m_ABCchips.rbegin()->first +1);
  }

  /// Add new ABC with given communications ID and HCC input channel
  void addABCchipID(unsigned int chipID, unsigned int hccIn) {
      m_ABCchips.emplace(hccIn, m_abc_version);
      m_ABCchips.at(hccIn).setABCChipId(chipID);
  }

  /// Remove all ABCs
  void clearABCchipIDs() { m_ABCchips.clear();}

  /// Set value of named register field (either ABC or HCC)
  void setSubRegisterValue(int chipIndex, std::string subRegName, uint32_t value);

  /// Get value of named register field (either ABC or HCC)
  uint32_t getSubRegisterValue(int chipIndex, std::string subRegName) const;

  /// Get register address for named register field (either ABC or HCC)
  int getSubRegisterParentAddr(int chipIndex, std::string subRegName);

  /// Get register value for named register field (either ABC or HCC)
  uint32_t getSubRegisterParentValue(int chipIndex, std::string subRegName);

  void maskPixel(unsigned col, unsigned row, bool doAltMask = false) override {}
  unsigned getPixelEn(unsigned col, unsigned row, bool doAltMask = false) override {
    return 1; // getPixelEn() was desgined for Pixels, further modification is needed for StarChip
  }
  void enableAll() override;

  /// Report mapping info to logger (for debugging)
  void logMappings() const;

  /// Is there an ABC associated with HCC input channel
  bool isAbcForInputChannel(int input_channel) const {
    assert(input_channel >= 0 && input_channel < HCC_INPUT_CHANNEL_COUNT);
    return (m_ABCchips.count(input_channel) > 0);
  }

  /// Return ABC associated with HCC input channel
  AbcCfg &abcForInputChannel(int hccIC) {
    return abcFromIndex(hccIC + 1);
  }

  /// Return ABC associated with HCC input channel
  const AbcCfg &abcForInputChannel(int hccIC) const {
    return abcFromIndex(hccIC + 1);
  }

  /// Is there an ABC associated with chip position in histogram.
  bool isAbcForHistoChip(int histo_chip) const;

  /// Return ABC associated with chip position in histogram.
  AbcCfg &abcForHistoChip(int histo_chip);

  /// Return ABC associated with chip position in histogram.
  const AbcCfg &abcForHistoChip(int histo_chip) const;

  /**
   * Obtain the corresponding charge [e] from the input VCal
   */
  double toCharge(double vcal) override;

  /**
   * Obtain the corresponding charge [e] from the input VCal, small&large capacitances(?)
   * Not fully implmented yet.
   */
  double toCharge(double vcal, bool sCap, bool lCap) override;

  /// Save configuration to json
  void writeConfig(json &j) override;

  /// Load configuration fromjson
  void loadConfig(const json &j) override;

  /// Generate set of configuration from template
  std::tuple<json, std::vector<json>> getPreset(const std::string& systemType) override;

  /// How many ABCs are attached
  size_t numABCs() { return m_ABCchips.size(); }

  /// Return highest input channel? of connected ABCs (internal?)
  int highestABC() const {
      if (m_ABCchips.size() == 0)
          return -1;
      return m_ABCchips.rbegin()->first; 
  } 

  /// Return lowest input channel? of connected ABCs (internal?)
  int lowestABC() const {
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

  /// Return HCC config
  HccCfg &hcc() { return m_hcc; }

  /// Return HCC config
  const HccCfg &hcc() const { return m_hcc; }

  /// Return HCC input channel for ABC communications ID
  int hccChannelForABCchipID(unsigned int chipID) const;

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

  /// HCC Fuse ID check used for communications ID for regist serial number set by eFuse bits
  uint32_t m_fuse_id=0;

  int m_abc_version; 
  int m_hcc_version; 

  HccCfg m_hcc;

  std::map<unsigned, AbcCfg> m_ABCchips;

  /// Return ABC via 1-based index into array (not for external use)
  AbcCfg &abcFromIndex(int chipIndex) {
    assert(isAbcForInputChannel(chipIndex-1));
    return m_ABCchips.at(chipIndex-1);
  }

  /// Return ABC via 1-based index into array (not for external use)
  const AbcCfg &abcFromIndex(int chipIndex) const {
    assert(chipIndex > 0);
    assert(isAbcForInputChannel(chipIndex-1));
    return m_ABCchips.at(chipIndex-1);
  }

  int inputChannelForHistoChip(int histo_chip) const;

  StarConversionTools m_ct;
};

#endif
