#ifndef HCC_STAR_CFG_INCLUDE
#define HCC_STAR_CFG_INCLUDE

#include <map>
#include <vector>

#include "enum.h"

#include "StarRegister.h"

static const size_t HCC_INPUT_CHANNEL_COUNT = 11;
/// No mapping for this input channel to histogram slot
static const size_t HCC_INPUT_CHANNEL_BAD_SLOT = 15;

//Different HCC registers that can be used
BETTER_ENUM(HCCStarRegister, int,
            SEU1=0, SEU2=1, SEU3=2, FrameRaw=3, LCBerr=4, ADCStatus=5, Status=6,
            HPR=15, Pulse=16, Addressing=17,
            Delay1=32, Delay2=33, Delay3=34,
            PLL1=35, PLL2=36, PLL3=37, DRV1=38, DRV2=39,
            ICenable=40, OPmode=41, OPmodeC=42, Cfg1=43, Cfg2=44,
            ExtRst=45, ExtRstC=46, ErrCfg=47, ADCcfg=48,

            // HCCV1: skip SEU3, PLL2, PLL3, adds:
            ABC_FLOW_1=7, ABC_FLOW_2=8, ABC_FLOW_3=9, READOUT_QUEUE=10
)

//Different HCC subregisters that can be used for configuration, scans, etc.
////NOTE: If the name is changed here, make sure the corresponding subregister name is also changed in the config json file.
BETTER_ENUM(HCCStarSubRegister, int,
            STOPHPR=1, TESTHPR,
            CFD_BC_FINEDELAY, CFD_BC_COARSEDELAY, CFD_PRLP_FINEDELAY, CFD_PRLP_COARSEDELAY, HFD_LCBA_FINEDELAY, FD_RCLK_FINEDELAY, LCBA_DELAY160,
            FD_DATAIN0_FINEDELAY, FD_DATAIN1_FINEDELAY, FD_DATAIN2_FINEDELAY, FD_DATAIN3_FINEDELAY, FD_DATAIN4_FINEDELAY, FD_DATAIN5_FINEDELAY, FD_DATAIN6_FINEDELAY, FD_DATAIN7_FINEDELAY,
            FD_DATAIN8_FINEDELAY, FD_DATAIN9_FINEDELAY, FD_DATAIN10_FINEDELAY,
            EPLLICP, EPLLCAP, EPLLRES, EPLLREFFREQ, EPLLENABLEPHASE,
            EPLLPHASE320A, EPLLPHASE320B, EPLLPHASE320C, EPLLPHASE160A, EPLLPHASE160B, EPLLPHASE160C,
            STVCLKOUTCUR, STVCLKOUTEN, LCBOUTCUR, LCBOUTEN, R3L1OUTCUR, R3L1OUTEN, BCHYBCUR, BCHYBEN, LCBAHYBCUR, LCBAHYBEN, PRLPHYBCUR, PRLPHYBEN, RCLKHYBCUR, RCLKHYBEN,
            DATA1CUR, DATA1ENPRE, DATA1ENABLE, DATA1TERM, DATACLKCUR, DATACLKENPRE, DATACLKENABLE,
            ICENABLE, ICTRANSSEL,
            TRIGMODE, ROSPEED, OPMODE, MAXNPACKETS, ENCODECNTL, ENCODE8B10B, PRBSMODE,
            TRIGMODEC, ROSPEEDC, OPMODEC, MAXNPACKETSC, ENCODECNTLC, ENCODE8B10BC, PRBSMODEC,
            BGSETTING, MASKHPR, GPO0, GPO1, EFUSEPROGBIT,
            BCIDRSTDELAY, BCMMSQUELCH,
            ABCRESETB, AMACSSSH, ABCRESETBC, AMACSSSHC,
            LCBERRCOUNTTHR, R3L1ERRCOUNTTHR,
            AMENABLE, AMCALIB, AMSW0, AMSW1, AMSW2, AMSW60, AMSW80, AMSW100, ANASET, THERMOFFSET,

            // HCCv1 drops:
            //  EPLLPHASE320A, EPLLPHASE320B, EPLLPHASE320C, EPLLPHASE160A, EPLLPHASE160B, EPLLPHASE160C,
            // AMSW0, AMSW1, AMSW2, AMSW60, AMSW80, AMSW100
            // Reg 35
            EPLLPHASE160,
            // Reg 41
            CLK_DIS_EN,
            // Reg 41
            CLK_DIS_ENC,
            // Reg 43
            BG_RANGE_LOW, BGVDD_CNT_EN, CLK_DIS_PHASE, CLK_DIS_SEL,
            // Reg 48
            AM_INT_SLOPE, AM_RANGE
)

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

        /// Return sub register info for name, throws std::runtime_error
        SubInfoPtr subRegByName(const std::string &subRegName) const {
          // This already throws runtime_error if bad string
          auto reg_enum = HCCStarSubRegister::_from_string(subRegName.c_str());
          return subRegFromEnum(reg_enum);
        }

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
        unsigned m_hccID = 0;

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

        const unsigned int getHCCchipID() const{return m_hccID;}
        void setHCCChipId(unsigned hccID){
            m_hccID = hccID;
        }

        void setSubRegisterValue(std::string subRegName, uint32_t value) {
            auto info = m_info->subRegByName(subRegName);
            m_registerMap.at(info->m_regAddress)->getSubRegister(info).updateValue(value);
        }

        uint32_t getSubRegisterValue(std::string subRegName) const {
            auto info = m_info->subRegByName(subRegName);
            return m_registerMap.at(info->m_regAddress)->getSubRegister(info).getValue();
        }

        int getSubRegisterParentAddr(std::string subRegName) const {
            auto info = m_info->subRegByName(subRegName);
            return info->getRegAddress();
        }

        uint32_t getSubRegisterParentValue(std::string subRegName) const {
            auto info = m_info->subRegByName(subRegName);
            return m_registerMap.at(info->m_regAddress)->getValue();
        }

        uint32_t getRegisterValue(HCCStarRegister addr) const;

        void setRegisterValue(HCCStarRegister addr, uint32_t val);

        std::array<uint8_t, HCC_INPUT_CHANNEL_COUNT> histoChipMap() const;

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
