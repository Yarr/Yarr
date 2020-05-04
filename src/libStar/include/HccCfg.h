#ifndef HCC_STAR_CFG_INCLUDE
#define HCC_STAR_CFG_INCLUDE

#include <map>
#include <vector>

#include "enum.h"

#include "StarRegister.h"

//Different HCC registers that can be used
BETTER_ENUM(HCCStarRegister, int,
            SEU1=0, SEU2=1, SEU3=2, FrameRaw=3, LCBerr=4, ADCStatus=5, Status=6,
            HPR=15, Pulse=16, Addressing=17,
            Delay1=32, Delay2=33, Delay3=34,
            PLL1=35, PLL2=36, PLL3=37, DRV1=38, DRV2=39,
            ICenable=40, OPmode=41, OPmodeC=42, Cfg1=43, Cfg2=44,
            ExtRst=45, ExtRstC=46, ErrCfg=47, ADCcfg=48)
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
            AMENABLE, AMCALIB, AMSW0, AMSW1, AMSW2, AMSW60, AMSW80, AMSW100, ANASET, THERMOFFSET)

/// Lookup information on HCCStar register map
class HccStarRegInfo {
    public:
        /// Fills the maps appropriately
        HccStarRegInfo();

        //This is a map from each register address to the register info.  Thus hccregisterMap[addr]
        std::map<unsigned, std::shared_ptr<RegisterInfo>> hccregisterMap;

        //This is a 2D map of each subregister to the HCC subregister name.  For example hccSubRegisterMap_all[NAME]
        // SubRegister is owned by the Registers
        std::map<HCCStarSubRegister, std::shared_ptr<SubRegisterInfo>> hccSubRegisterMap_all;   //register record

        static std::shared_ptr<HccStarRegInfo> instance() {
            if(!m_instance) m_instance.reset(new HccStarRegInfo);
            return m_instance;
        }
   private:
        static std::shared_ptr<HccStarRegInfo> m_instance;
};

/// Configuration for an individual HCCStar
class HccCfg {
        unsigned m_hccID;

        //This is a map from address to register (pointers into register set)
        std::map<unsigned, Register*> m_registerMap;

        // Store of registers in arbitrary order
        std::vector< Register > m_registerSet;

        std::shared_ptr< HccStarRegInfo > m_info;

    public:
        HccCfg();

        HccCfg(const HccCfg &) = delete;
        HccCfg &operator =(const HccCfg &) = delete;
        HccCfg &operator =(HccCfg &&) = delete;
        // Default doesn't work as won't change pointers!
        HccCfg(HccCfg &&other) = delete;

        void setDefaults();

        const unsigned int getHCCchipID(){return m_hccID;}
        void setHCCChipId(unsigned hccID){
            m_hccID = hccID;
        }

        void setSubRegisterValue(std::string subRegName, uint32_t value) {
            auto info = m_info->hccSubRegisterMap_all[HCCStarSubRegister::_from_string(subRegName.c_str())];
            m_registerMap.at(info->m_regAddress)->getSubRegister(info).updateValue(value);
        }

        uint32_t getSubRegisterValue(std::string subRegName) {
            auto info = m_info->hccSubRegisterMap_all[HCCStarSubRegister::_from_string(subRegName.c_str())];
            return m_registerMap.at(info->m_regAddress)->getSubRegister(info).getValue();
        }

        int getSubRegisterParentAddr(std::string subRegName) const {
            auto info = m_info->hccSubRegisterMap_all[HCCStarSubRegister::_from_string(subRegName.c_str())];
            return info->getRegAddress();
        }

        uint32_t getSubRegisterParentValue(std::string subRegName) const {
            auto info = m_info->hccSubRegisterMap_all[HCCStarSubRegister::_from_string(subRegName.c_str())];
            return m_registerMap.at(info->m_regAddress)->getValue();
        }

        uint32_t getRegisterValue(HCCStarRegister addr) const;

        void setRegisterValue(HCCStarRegister addr, uint32_t val);

    private:
        SubRegister getSubRegister(HCCStarSubRegister r) const {
            auto info = m_info->hccSubRegisterMap_all[r];
            return m_registerMap.at(info->m_regAddress)->getSubRegister(info);
        }

        const Register &getRegister(HCCStarRegister addr) const {
            return *m_registerMap.at((unsigned int)addr);
        }

        Register &getRegister(HCCStarRegister addr) {
            return *m_registerMap.at((unsigned int)addr);
        }

        void setupMaps();
};


#endif
