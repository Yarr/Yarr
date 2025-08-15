#ifndef HCC_STAR_CFG_INCLUDE
#define HCC_STAR_CFG_INCLUDE

#include <map>
#include <vector>

#include "StarConstants.h"
#include "StarRegister.h"
#include "StarRegDefs.h"

/// No mapping for this input channel to histogram slot
static const size_t HCC_INPUT_CHANNEL_BAD_SLOT = 15;

//Different HCC registers that can be used
enum class HCCStarRegister {
  HCC_STAR_REGS
};

//Different HCC subregisters that can be used for configuration, scans, etc.
////NOTE: If the name is changed here, make sure the corresponding subregister name is also changed in the config json file.
enum class HCCStarSubRegister {
  HCC_STAR_SUB_REGS
};

/// Lookup information on HCCStar register map
class HccStarRegInfo {
        typedef std::shared_ptr<const RegisterInfo> InfoPtr;
        typedef std::shared_ptr<const SubRegisterInfo> SubInfoPtr;

    public:
        /// Fills the maps appropriately
        HccStarRegInfo(int version);

        //This is a map from each register address to the register info.  Thus hccregisterMap[addr]
        std::map<unsigned, InfoPtr> hccregisterMap;

        /// The list of registers to write to
        std::map<unsigned, InfoPtr> hccWriteMap;

        /// Return sub register info from enum, throws std::runtime_error
        SubInfoPtr subRegFromEnum(HCCStarSubRegister subReg) const {
          try {
            return hccSubRegisterMap_all.at(subReg);
          } catch(std::out_of_range &e) {
            throw std::runtime_error("Attempt to get info for bad (HCC) sub-register");
          }
        }

        /// The sub-reg map is accessible, but is const so can't be updated
        const std::map<HCCStarSubRegister, SubInfoPtr> subRegMap() const {
            return hccSubRegisterMap_all;
        }

        static std::shared_ptr<const HccStarRegInfo> instance(int version);

   private:
        /// Map of each subregister (enum) to the HCC subregister info
        std::map<HCCStarSubRegister, SubInfoPtr> hccSubRegisterMap_all;

        static std::shared_ptr<const HccStarRegInfo> m_instance;
};

/// Configuration for an individual HCCStar
class HccCfg {
        unsigned m_hccID=0;

        //This is a map from address to register (pointers into register set)
        std::map<unsigned, Register*> m_registerMap;

        // Store of registers in arbitrary order
        std::vector< Register > m_registerSet;

        std::shared_ptr<const HccStarRegInfo > m_info;

    public:
        HccCfg(int version);

        HccCfg() = delete;
        ~HccCfg() = default;
        // Remove these as difficult to update pointers in m_registerMap!
        HccCfg(const HccCfg &) = delete;
        HccCfg &operator =(const HccCfg &) = delete;
        HccCfg &operator =(HccCfg &&) = delete;
        HccCfg(HccCfg &&other) = delete;

        void setDefaults(int version);

        /// Get communications ID for this HCC
        const unsigned int getHCCchipID() const{return m_hccID;}
        /// Set communications ID for this HCC
        void setHCCChipId(unsigned hccID){
            m_hccID = hccID;
        }

        /// Set value of register field
        void setSubRegisterValue(HCCStarSubRegister subReg, uint32_t value) {
            auto info = m_info->subRegFromEnum(subReg);
            m_registerMap.at(info->m_regAddress)->getSubRegister(info).updateValue(value);
        }

        /// Get value of register field
        uint32_t getSubRegisterValue(HCCStarSubRegister subReg) const {
            auto info = m_info->subRegFromEnum(subReg);
            return m_registerMap.at(info->m_regAddress)->getSubRegister(info).getValue();
        }

        /// Retrieve address of register corresponding to register field
        int getSubRegisterParentAddr(HCCStarSubRegister subReg) const {
            auto info = m_info->subRegFromEnum(subReg);
            return info->getRegAddress();
        }

        /// Retrieve full value of register containing register field
        uint32_t getSubRegisterParentValue(HCCStarSubRegister subReg) const {
            auto info = m_info->subRegFromEnum(subReg);
            return m_registerMap.at(info->m_regAddress)->getValue();
        }

        /// Get configured value of register address
        uint32_t getRegisterValue(HCCStarRegister addr) const;

        /// Set configured value of register address
        void setRegisterValue(HCCStarRegister addr, uint32_t val);

        /**
           Map from input channels to histogram location.

           This is used by StarDataProcessor to put hits corresponding to
           a particular ABC into the correct location in the histogram.

           This is not configurable directly, but is calculated based on
           the HCC version and the contents of the chip enables register.
        */
        std::array<uint8_t, Star::MaxABCsPerHCC> histoChipMap() const;

    private:
        SubRegister getSubRegister(HCCStarSubRegister r) const {
            auto info = m_info->subRegFromEnum(r);
            return m_registerMap.at(info->m_regAddress)->getSubRegister(info);
        }

        const Register &getRegister(HCCStarRegister addr) const {
            return *m_registerMap.at((unsigned int)addr);
        }

        Register &getRegister(HCCStarRegister addr) {
            return *m_registerMap.at((unsigned int)addr);
        }

        void setupMaps(int version);
};


#endif
