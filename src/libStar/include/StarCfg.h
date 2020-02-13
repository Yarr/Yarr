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
#include "enum.h"

#include "StarRegister.h"

//Different HCC and ABC registers that can be used
BETTER_ENUM(HCCStarRegister, int,
            Pulse=16, Delay1=32, Delay2=33, Delay3=34,
            PLL1=35, PLL2=36, PLL3=37, DRV1=38, DRV2=39,
            ICenable=40, OPmode=41, OPmodeC=42, Cfg1=43, Cfg2=44,
            ExtRst=45, ExtRstC=46, ErrCfg=47, ADCcfg=48)
BETTER_ENUM(ABCStarRegs, int,
            SCReg=0, ADCS1=1, ADCS2=2, ADCS3=3, ADCS4=4, ADCS5=5, ADCS6=6, ADCS7=7,
            MaskInput0=16, MaskInput1=17, MaskInput2=18, MaskInput3=19, MaskInput4=20, MaskInput5=21, MaskInput6=22, MaskInput7=23,
            CREG0=32, CREG1=33, CREG2=34, CREG3=35, CREG4=36, CREG5=37, CREG6=38,
            TrimDAC0=64, TrimDAC1=65, TrimDAC2=66, TrimDAC3=67, TrimDAC4=68, TrimDAC5=69, TrimDAC6=70, TrimDAC7=71, TrimDAC8=72, TrimDAC9=73, TrimDAC10=74, TrimDAC11=75, TrimDAC12=76, TrimDAC13=77, TrimDAC14=78, TrimDAC15=79, TrimDAC16=80, TrimDAC17=81, TrimDAC18=82, TrimDAC19=83, TrimDAC20=84, TrimDAC21=85, TrimDAC22=86, TrimDAC23=87, TrimDAC24=88, TrimDAC25=89, TrimDAC26=90, TrimDAC27=91, TrimDAC28=92, TrimDAC29=93, TrimDAC30=94, TrimDAC31=95, TrimDAC32=96, TrimDAC33=97, TrimDAC34=98, TrimDAC35=99, TrimDAC36=100, TrimDAC37=101, TrimDAC38=102, TrimDAC39=103,
            CalREG0=104, CalREG1=105, CalREG2=106, CalREG3=107, CalREG4=108, CalREG5=109, CalREG6=110, CalREG7=111)
//Different HCC and ABC subregisters that can be used for configuration, scans, etc.
////NOTE: If the name is changed here, make sure the corresponding subregister name is also changed in the config json file.
BETTER_ENUM(HCCStarSubRegister, int,
            STOPHPR=1,
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
 ABCStarRegister(const ABCStarRegs & other) : ABCStarRegs::ABCStarRegs(other) {};
  static  ABCStarRegister MaskInput(int i) { return ABCStarRegs::_from_integral((int)(ABCStarRegs::MaskInput0) + i);};
};

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
  std::map<std::string, std::shared_ptr<SubRegisterInfo>> trimDAC_4LSB_RegisterMap_all;

  //This is a 2D map of each trimDac_32b register to the chip index and trimDAC_1MSB register name.  For example trimDAC1LSB_RegisterMap_all[chip index][NAME]
  std::map<std::string, std::shared_ptr<SubRegisterInfo>> trimDAC_1MSB_RegisterMap_all;

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
