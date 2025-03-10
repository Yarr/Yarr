#ifndef ABC_STAR_CFG_INCLUDE
#define ABC_STAR_CFG_INCLUDE

// #################################
// # Project:
// # Description: StarChips Library
// # Comment: ABC Star configuration class
// ################################

#include <map>
#include <memory>
#include <vector>

#include "StarRegDefs.h"
#include "StarRegister.h"

// Name different ABC registers that can be used
//  NB some are not used in V1
enum class ABCStarRegister {
  ABC_STAR_REGS
};

// Add prefix operator so we can iterate
// NB: Only to be uses for valid ranges
inline ABCStarRegister operator ++(ABCStarRegister &r) {
  r = ((ABCStarRegister) ((int)r + 1));
  return r;
}

//Name different ABCv0 subregisters that can be used for configuration, scans, etc.
enum class ABCStarSubRegister {
  ABC_STAR_SUB_REGS
};

/// Representation of the address of an ABCStar register
namespace ABCStarRegisters {
    static  ABCStarRegister MaskInput(int i) { return (ABCStarRegister)((int)(ABCStarRegister::MaskInput0) + i);}
    static  ABCStarRegister CalReg(int i) { return (ABCStarRegister)((int)(ABCStarRegister::CalREG0) + i);}
    static  ABCStarRegister Counter(int i) { return (ABCStarRegister)((int)(ABCStarRegister::HitCountREG0) + i);}
    /// 32 registers containing lo 4 bits of trim
    static  ABCStarRegister TrimLo(int i) { return (ABCStarRegister)((int)(ABCStarRegister::TrimDAC0) + i);}
    /// 8 registers containing hi 1 bits of trim
    static  ABCStarRegister TrimHi(int i) { return (ABCStarRegister)((int)(ABCStarRegister::TrimDAC32) + i);}
};

/// Lookup information on ABC Star register map
class AbcStarRegInfo {
  typedef std::shared_ptr<const RegisterInfo> InfoPtr;
  typedef std::shared_ptr<const SubRegisterInfo> SubInfoPtr;

  public:
  /// Fills the maps appropriately
  AbcStarRegInfo(int version);

  //This is a map from each register address to the register info.  Thus abcregisterMap[addr]
  std::map<unsigned, InfoPtr> abcregisterMap;

  /// Registers to write in normal operation
  std::map<unsigned, InfoPtr> abcWriteMap;

  static std::shared_ptr<const AbcStarRegInfo> instance(int version);

  /// Return sub register info for name, throws std::runtime_error
  SubInfoPtr subRegByName(const std::string &subRegName) const;

  /// Return sub register info from enum, throws std::runtime_error
  SubInfoPtr subRegFromEnum(ABCStarSubRegister subReg) const;

  int getSubRegisterParentAddr(std::string subRegName) const {
    return subRegByName(subRegName)->getRegAddress();
  }

  /// The sub-reg map is accessible, but is const so can't be updated
  const std::map<ABCStarSubRegister, SubInfoPtr> subRegMap() const {
    return abcSubRegisterMap_all;
  }

 private:
  /// This is a map from each subregister to the ABC subregister name
  /// For example abcSubRegisterMap_all[NAME]
  std::map<ABCStarSubRegister, SubInfoPtr> abcSubRegisterMap_all;
};

/// Configuration for an individual ABCStar
class AbcCfg {
        unsigned m_abcID;

        //This is a map from address to register index (into register set)
        std::map<unsigned, unsigned> m_registerMap;

        // Store of registers in arbitrary order
        std::vector< Register > m_registerSet;

        std::shared_ptr<const AbcStarRegInfo > m_info;

    public:
        AbcCfg(int version);

        AbcCfg() = delete;
        ~AbcCfg() = default;

        // Note that the default version should work but removed so we check on usage
        AbcCfg(const AbcCfg &) = delete;
        AbcCfg &operator =(const AbcCfg &) = delete;
        AbcCfg &operator =(AbcCfg &&) = delete;
        AbcCfg(AbcCfg &&other) = delete;

        /// Set default register values for version 0 or 1
        void setDefaults(int version);

        /// Get the ID used to communicate with this ABC
        unsigned int getABCchipID() const { return m_abcID;}

        /// Set the ID used to communicate with this ABC
        void setABCChipId(unsigned abcID){
            m_abcID = abcID;
        }

        /// Set the value of a register field for this ABC
        void setSubRegisterValue(std::string subRegName, uint32_t value) {
            auto info = m_info->subRegByName(subRegName);
            auto &reg = getRegister(info->m_regAddress);
            reg.getSubRegister(info).updateValue(value);
        }

        /// Get the value of a register field for this ABC
        uint32_t getSubRegisterValue(std::string subRegName) const {
            auto info = m_info->subRegByName(subRegName);
            auto &reg = getRegister(info->m_regAddress);
            return reg.getSubRegister(info).getValue();
        }

        /// Lookup the register address for a named register field
        int getSubRegisterParentAddr(std::string subRegName) const {
            return m_info->getSubRegisterParentAddr(subRegName);
        }

        /// Find the full register contents for a named register field
        uint32_t getSubRegisterParentValue(std::string subRegName) const {
            auto info = m_info->subRegByName(subRegName);
            return getRegister(info->m_regAddress).getValue();
        }

        /// Find the full register contents for a register address
        uint32_t getRegisterValue(ABCStarRegister addr) const;

        /// Set register contents
        void setRegisterValue(ABCStarRegister addr, uint32_t val);

        /// Set trim DAC for particular channel (as calculated by StarCfg)
        void setTrimDACRaw(unsigned channel, int value);

        /// Get trim DAC for particular channel (as calculated by StarCfg)
        int getTrimDACRaw(unsigned channel) const;

        /**
         * Convert from strip order to ordering as in the trim registers.
         */
        uint8_t trimRegOrderFromChannel(uint8_t chn) const {
            return ((chn & 0x7e) << 1) | (chn & 0x1) | ((chn & 0x80) >> 6);
        }

        /// Is channel masked
        bool isMasked(unsigned channel) const {
            uint8_t maskIndex = ((channel & 0x7f) << 1) | ((channel & 0x80) >> 7);
            auto maskReg = ABCStarRegisters::MaskInput((maskIndex>>5) & 0x7);
            uint32_t maskValue = getRegister((int)maskReg).getValue();
            return maskValue & (1 << (maskIndex&0x1f));
        }

        /// Set mask for strip
        void setMask(unsigned channel, bool mask) {
            uint8_t maskIndex = ((channel & 0x7f) << 1) | ((channel & 0x80) >> 7);
            auto maskReg = ABCStarRegisters::MaskInput((maskIndex>>5) & 0x7);
            auto &reg = getRegister((int)maskReg);
            uint32_t maskValue = reg.getValue();
            uint32_t maskPattern =  1 << (maskIndex&0x1f);

            if(mask) {
              maskValue |= maskPattern; 
            } else {
              maskValue &= ~maskPattern; 
            }
            reg.setValue(maskValue);
        }

    private:
        SubRegister getSubRegister(ABCStarSubRegister r) {
            auto info = m_info->subRegFromEnum(r);
            return getRegister(info->m_regAddress).getSubRegister(info);
        }

        ConstSubRegister getSubRegister(ABCStarSubRegister r) const {
            auto info = m_info->subRegFromEnum(r);
            return getRegister(info->m_regAddress).getSubRegister(info);
        }

        const Register &getRegister(ABCStarRegister addr) const {
            auto index = m_registerMap.at((unsigned int)addr);
            return m_registerSet[index];
        }

        Register &getRegister(ABCStarRegister addr) {
            auto index = m_registerMap.at((unsigned int)addr);
            return m_registerSet[index];
        }

        const Register &getRegister(unsigned int addr) const {
            auto index = m_registerMap.at(addr);
            return m_registerSet[index];
        }

        Register &getRegister(unsigned int addr) {
            auto index = m_registerMap.at(addr);
            return m_registerSet[index];
        }

        void setupMaps(int version);
};

#endif
