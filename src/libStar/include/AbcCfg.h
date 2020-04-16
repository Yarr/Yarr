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

#include "enum.h"
#include "StarRegister.h"

// Name different ABC registers that can be used
BETTER_ENUM(ABCStarRegs, int,
            SCReg=0, ADCS1=1, ADCS2=2, ADCS3=3, ADCS4=4, ADCS5=5, ADCS6=6, ADCS7=7,
            MaskInput0=16, MaskInput1=17, MaskInput2=18, MaskInput3=19, MaskInput4=20, MaskInput5=21, MaskInput6=22, MaskInput7=23,
            CREG0=32, CREG1=33, CREG2=34, CREG3=35, CREG4=36,
            /* CREG5=37,  */ // This is used for fusing only
            CREG6=38,
            STAT0=48, STAT1=49, STAT2=50, STAT3=51, STAT4=52, HPR=63,
            TrimDAC0=64, TrimDAC1=65, TrimDAC2=66, TrimDAC3=67, TrimDAC4=68, TrimDAC5=69, TrimDAC6=70, TrimDAC7=71, TrimDAC8=72, TrimDAC9=73, TrimDAC10=74, TrimDAC11=75, TrimDAC12=76, TrimDAC13=77, TrimDAC14=78, TrimDAC15=79, TrimDAC16=80, TrimDAC17=81, TrimDAC18=82, TrimDAC19=83, TrimDAC20=84, TrimDAC21=85, TrimDAC22=86, TrimDAC23=87, TrimDAC24=88, TrimDAC25=89, TrimDAC26=90, TrimDAC27=91, TrimDAC28=92, TrimDAC29=93, TrimDAC30=94, TrimDAC31=95, TrimDAC32=96, TrimDAC33=97, TrimDAC34=98, TrimDAC35=99, TrimDAC36=100, TrimDAC37=101, TrimDAC38=102, TrimDAC39=103,
            CalREG0=104, CalREG1=105, CalREG2=106, CalREG3=107, CalREG4=108, CalREG5=109, CalREG6=110, CalREG7=111,
            HitCountREG0=128, HitCountREG1=129, HitCountREG2=130, HitCountREG3=131, HitCountREG4=132, HitCountREG5=133, HitCountREG6=134, HitCountREG7=135, HitCountREG8=136, HitCountREG9=137, HitCountREG10=138, HitCountREG11=139, HitCountREG12=140, HitCountREG13=141, HitCountREG14=142, HitCountREG15=143, HitCountREG16=144, HitCountREG17=145, HitCountREG18=146, HitCountREG19=147, HitCountREG20=148, HitCountREG21=149, HitCountREG22=150, HitCountREG23=151, HitCountREG24=152, HitCountREG25=153, HitCountREG26=154, HitCountREG27=155, HitCountREG28=156, HitCountREG29=157, HitCountREG30=158, HitCountREG31=159, HitCountREG32=160, HitCountREG33=161, HitCountREG34=162, HitCountREG35=163, HitCountREG36=164, HitCountREG37=165, HitCountREG38=166, HitCountREG39=167, HitCountREG40=168, HitCountREG41=169, HitCountREG42=170, HitCountREG43=171, HitCountREG44=172, HitCountREG45=173, HitCountREG46=174, HitCountREG47=175, HitCountREG48=176, HitCountREG49=177, HitCountREG50=178, HitCountREG51=179, HitCountREG52=180, HitCountREG53=181, HitCountREG54=182, HitCountREG55=183, HitCountREG56=184, HitCountREG57=185, HitCountREG58=186, HitCountREG59=187, HitCountREG60=188, HitCountREG61=189, HitCountREG62=190, HitCountREG63=191)

//Name different ABC subregisters that can be used for configuration, scans, etc.

BETTER_ENUM(ABCStarSubRegister, int,
            RRFORCE=1, WRITEDISABLE, STOPHPR, TESTHPR, EFUSEL, LCBERRCNTCLR,
            BVREF, BIREF, B8BREF, BTRANGE,
            BVT, COMBIAS, BIFEED, BIPRE,
            STR_DEL_R, STR_DEL, BCAL, BCAL_RANGE,
            ADC_BIAS, ADC_CH, ADC_ENABLE,
            D_S, D_LOW, D_EN_CTRL,
            BTMUX, BTMUXD, A_S, A_EN_CTRL,
            TEST_PULSE_ENABLE, ENCOUNT, MASKHPR, PR_ENABLE, LP_ENABLE, RRMODE, TM, TESTPATT_ENABLE, TESTPATT1, TESTPATT2,
            CURRDRIV, CALPULSE_ENABLE, CALPULSE_POLARITY,
            LATENCY, BCFLAG_ENABLE, BCOFFSET,
            DETMODE, MAX_CLUSTER, MAX_CLUSTER_ENABLE,
            EN_CLUSTER_EMPTY, EN_CLUSTER_FULL, EN_CLUSTER_OVFL, EN_REGFIFO_EMPTY, EN_REGFIFO_FULL, EN_REGFIFO_OVFL, EN_LPFIFO_EMPTY, EN_LPFIFO_FULL, EN_PRFIFO_EMPTY, EN_PRFIFO_FULL, EN_LCB_LOCKED, EN_LCB_DECODE_ERR, EN_LCB_ERRCNT_OVFL, EN_LCB_SCMD_ERR,
            // DOFUSE, // Not needed
            LCB_ERRCOUNT_THR)

class ABCStarRegister : public ABCStarRegs {
  public:
    ABCStarRegister(const ABCStarRegs & other) : ABCStarRegs::ABCStarRegs(other) {}
    ABCStarRegister(const ABCStarRegs::_enumerated & other) : ABCStarRegs::ABCStarRegs(other) {}
    static  ABCStarRegister MaskInput(int i) { return ABCStarRegs::_from_integral((int)(ABCStarRegs::MaskInput0) + i);}
    /// 32 registers containing lo 4 bits of trim
    static  ABCStarRegister TrimLo(int i) { return ABCStarRegs::_from_integral((int)(ABCStarRegs::TrimDAC0) + i);}
    /// 8 registers containing hi 1 bits of trim
    static  ABCStarRegister TrimHi(int i) { return ABCStarRegs::_from_integral((int)(ABCStarRegs::TrimDAC32) + i);}
};

/// Lookup information on ABC Star register map
class AbcStarRegInfo {
  public:
  /// Fills the maps appropriately
  AbcStarRegInfo();

  //This is a map from each register address to the register info.  Thus abcregisterMap[addr]
  std::map<unsigned, std::shared_ptr<RegisterInfo>> abcregisterMap;

  //This is a 2D map of each subregister to the ABC subregister name.  For example abcSubRegisterMap_all[NAME]
  std::map<ABCStarSubRegister, std::shared_ptr<SubRegisterInfo>> abcSubRegisterMap_all;

  //This is a 2D map of each trimDac_32b register to the chip index and trimDAC_4LSB register name.  For example trimDAC4LSB_RegisterMap_all[chip index][NAME]
  std::map<int, std::shared_ptr<SubRegisterInfo>> trimDAC_4LSB_RegisterMap_all;

  //This is a 2D map of each trimDac_32b register to the chip index and trimDAC_1MSB register name.  For example trimDAC1LSB_RegisterMap_all[chip index][NAME]
  std::map<int, std::shared_ptr<SubRegisterInfo>> trimDAC_1MSB_RegisterMap_all;

  static std::shared_ptr<AbcStarRegInfo> instance() {
    if(!m_instance) m_instance.reset(new AbcStarRegInfo);
    return m_instance;
  }

  int getSubRegisterParentAddr(std::string subRegName) const {
    auto info = abcSubRegisterMap_all.at(ABCStarSubRegister::_from_string(subRegName.c_str()));
    return info->getRegAddress();
  }
 private:
  static std::shared_ptr<AbcStarRegInfo> m_instance;
};

/// Configuration for an individual ABCStar
class AbcCfg {
        unsigned m_abcID;

        //This is a map from address to register index (into register set)
        std::map<unsigned, unsigned> m_registerMap;

        // Store of registers in arbitrary order
        std::vector< Register > m_registerSet;

        std::shared_ptr< AbcStarRegInfo > m_info;

    public:
        AbcCfg();
        AbcCfg(const AbcCfg &) = delete;
        AbcCfg &operator =(const AbcCfg &) = delete;
        AbcCfg &operator =(AbcCfg &&) = delete;
        // Default move works (no pointers)
        AbcCfg(AbcCfg &&other) = default;

        void setDefaults();

        unsigned int getABCchipID() const { return m_abcID;}
        void setABCChipId(unsigned abcID){
            m_abcID = abcID;
        }

        void setSubRegisterValue(std::string subRegName, uint32_t value) {
            auto info = m_info->abcSubRegisterMap_all[ABCStarSubRegister::_from_string(subRegName.c_str())];
            auto &reg = getRegister(info->m_regAddress);
            reg.getSubRegister(info).updateValue(value);
        }

        uint32_t getSubRegisterValue(std::string subRegName) {
            auto info = m_info->abcSubRegisterMap_all[ABCStarSubRegister::_from_string(subRegName.c_str())];
            auto &reg = getRegister(info->m_regAddress);
            return reg.getSubRegister(info).getValue();
        }

        int getSubRegisterParentAddr(std::string subRegName) const {
            return m_info->getSubRegisterParentAddr(subRegName);
        }

        uint32_t getSubRegisterParentValue(std::string subRegName) const {
            auto info = m_info->abcSubRegisterMap_all[ABCStarSubRegister::_from_string(subRegName.c_str())];
            return getRegister(info->m_regAddress).getValue();
        }

        uint32_t getRegisterValue(ABCStarRegister addr) const;

        void setRegisterValue(ABCStarRegister addr, uint32_t val);

        /// Set trim DAC for particular channel (as calculated by StarCfg)
        void setTrimDACRaw(unsigned channel, int value);

        /// Get trim DAC for particular channel (as calculated by StarCfg)
        int getTrimDACRaw(unsigned channel) const;

        /// Is channel masked
        bool isMasked(unsigned channel) const {
            uint8_t maskIndex = ((channel & 0x7f) << 1) | ((channel & 0x80) >> 7);
            int maskReg = ABCStarRegister::MaskInput((maskIndex>>5) & 0x7);
            uint32_t maskValue = getRegister(maskReg).getValue();
            return maskValue & (1 << (maskIndex&0x1f));
        }

        /// Set mask for strip
        void setMask(unsigned channel, bool mask) {
            uint8_t maskIndex = ((channel & 0x7f) << 1) | ((channel & 0x80) >> 7);
            int maskReg = ABCStarRegister::MaskInput((maskIndex>>5) & 0x7);
            auto &reg = getRegister(maskReg);
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
            auto info = m_info->abcSubRegisterMap_all[r];
            return getRegister(info->m_regAddress).getSubRegister(info);
        }

        ConstSubRegister getSubRegister(ABCStarSubRegister r) const {
            auto info = m_info->abcSubRegisterMap_all[r];
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

        void setupMaps();
};

#endif
