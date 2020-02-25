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
            CREG0=32, CREG1=33, CREG2=34, CREG3=35, CREG4=36, CREG5=37, CREG6=38,
            STAT0=48, STAT1=49, STAT2=50, STAT3=51, STAT4=52, HPR=63,
            TrimDAC0=64, TrimDAC1=65, TrimDAC2=66, TrimDAC3=67, TrimDAC4=68, TrimDAC5=69, TrimDAC6=70, TrimDAC7=71, TrimDAC8=72, TrimDAC9=73, TrimDAC10=74, TrimDAC11=75, TrimDAC12=76, TrimDAC13=77, TrimDAC14=78, TrimDAC15=79, TrimDAC16=80, TrimDAC17=81, TrimDAC18=82, TrimDAC19=83, TrimDAC20=84, TrimDAC21=85, TrimDAC22=86, TrimDAC23=87, TrimDAC24=88, TrimDAC25=89, TrimDAC26=90, TrimDAC27=91, TrimDAC28=92, TrimDAC29=93, TrimDAC30=94, TrimDAC31=95, TrimDAC32=96, TrimDAC33=97, TrimDAC34=98, TrimDAC35=99, TrimDAC36=100, TrimDAC37=101, TrimDAC38=102, TrimDAC39=103,
            CalREG0=104, CalREG1=105, CalREG2=106, CalREG3=107, CalREG4=108, CalREG5=109, CalREG6=110, CalREG7=111,
            HitCountREG0=112, HitCountREG1=113, HitCountREG2=114, HitCountREG3=115, HitCountREG4=116, HitCountREG5=117, HitCountREG6=118, HitCountREG7=119, HitCountREG8=120, HitCountREG9=121, HitCountREG10=122, HitCountREG11=123, HitCountREG12=124, HitCountREG13=125, HitCountREG14=126, HitCountREG15=127, HitCountREG16=128, HitCountREG17=129, HitCountREG18=130, HitCountREG19=131, HitCountREG20=132, HitCountREG21=133, HitCountREG22=134, HitCountREG23=135, HitCountREG24=136, HitCountREG25=137, HitCountREG26=138, HitCountREG27=139, HitCountREG28=140, HitCountREG29=141, HitCountREG30=142, HitCountREG31=143, HitCountREG32=144, HitCountREG33=145, HitCountREG34=146, HitCountREG35=147, HitCountREG36=148, HitCountREG37=149, HitCountREG38=150, HitCountREG39=151, HitCountREG40=152, HitCountREG41=153, HitCountREG42=154, HitCountREG43=155, HitCountREG44=156, HitCountREG45=157, HitCountREG46=158, HitCountREG47=159, HitCountREG48=160, HitCountREG49=161, HitCountREG50=162, HitCountREG51=163, HitCountREG52=164, HitCountREG53=165, HitCountREG54=166, HitCountREG55=167, HitCountREG56=168, HitCountREG57=169, HitCountREG58=170, HitCountREG59=171, HitCountREG60=172, HitCountREG61=173, HitCountREG62=174, HitCountREG63=175
  )

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
            DOFUSE,
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

        //This is a map from address to register (pointers into register set)
        std::map<unsigned, Register*> m_registerMap;

        // Store of registers in arbitrary order
        std::vector< Register > m_registerSet;

        std::shared_ptr< AbcStarRegInfo > m_info;

    public:
        AbcCfg();

        void configure_ABC_Registers();

        unsigned int getABCchipID() const { return m_abcID;}
        void setABCChipId(unsigned abcID){
            m_abcID = abcID;
        }

        void setSubRegisterValue(std::string subRegName, uint32_t value) {
            auto info = m_info->abcSubRegisterMap_all[ABCStarSubRegister::_from_string(subRegName.c_str())];
            m_registerMap.at(info->m_regAddress)->getSubRegister(info).updateValue(value);
        }

        uint32_t getSubRegisterValue(std::string subRegName) {
            auto info = m_info->abcSubRegisterMap_all[ABCStarSubRegister::_from_string(subRegName.c_str())];
            return m_registerMap.at(info->m_regAddress)->getSubRegister(info).getValue();
        }

        int getSubRegisterParentAddr(std::string subRegName) const {
            return m_info->getSubRegisterParentAddr(subRegName);
        }

        uint32_t getSubRegisterParentValue(std::string subRegName) const {
            auto info = m_info->abcSubRegisterMap_all[ABCStarSubRegister::_from_string(subRegName.c_str())];
            return m_registerMap.at(info->m_regAddress)->getValue();
        }

        uint32_t getRegisterValue(ABCStarRegister addr) const;

        void setRegisterValue(ABCStarRegister addr, uint32_t val);

        /// Set trim DAC for particular channel (as calculated by StarCfg)
        void setTrimDACRaw(unsigned channel, int value);

        /// Get trim DAC for particular channel (as calculated by StarCfg)
        int getTrimDACRaw(unsigned channel) const;

    private:
        SubRegister getSubRegister(ABCStarSubRegister r) const {
            auto info = m_info->abcSubRegisterMap_all[r];
            return m_registerMap.at(info->m_regAddress)->getSubRegister(info);
        }

        const Register &getRegister(ABCStarRegister addr) const {
            return *m_registerMap.at((unsigned int)addr);
        }

        Register &getRegister(ABCStarRegister addr) {
            return *m_registerMap.at((unsigned int)addr);
        }
};

#endif
