#include "HccCfg.h"

#include "logging.h"

namespace {
  auto logger = logging::make_log("StarCfgHCC");
}

std::shared_ptr<HccStarRegInfo> HccStarRegInfo::m_instance;

typedef std::tuple<HCCStarSubRegister, unsigned int, unsigned int, unsigned int> hccsubregdef;
const std::vector<hccsubregdef> s_hccsubregdefs = {
  {HCCStarSubRegister::STOPHPR			,16	,0	,1}	,
  {HCCStarSubRegister::TESTHPR			,16	,1	,1}	,
  {HCCStarSubRegister::CFD_BC_FINEDELAY		,32	,0	,4}	,
  {HCCStarSubRegister::CFD_BC_COARSEDELAY	,32	,4	,2}	,
  {HCCStarSubRegister::CFD_PRLP_FINEDELAY	,32	,8	,4}	,
  {HCCStarSubRegister::CFD_PRLP_COARSEDELAY	,32	,12	,2}	,
  {HCCStarSubRegister::HFD_LCBA_FINEDELAY	,32	,16	,4}	,
  {HCCStarSubRegister::FD_RCLK_FINEDELAY	,32	,20	,4}	,
  {HCCStarSubRegister::LCBA_DELAY160            ,32	,24	,2}	,
  {HCCStarSubRegister::FD_DATAIN0_FINEDELAY     ,33	,0	,4}	,
  {HCCStarSubRegister::FD_DATAIN1_FINEDELAY     ,33	,4	,4}	,
  {HCCStarSubRegister::FD_DATAIN2_FINEDELAY     ,33	,8	,4}	,
  {HCCStarSubRegister::FD_DATAIN3_FINEDELAY     ,33	,12	,4}	,
  {HCCStarSubRegister::FD_DATAIN4_FINEDELAY     ,33	,16	,4}	,
  {HCCStarSubRegister::FD_DATAIN5_FINEDELAY     ,33	,20	,4}	,
  {HCCStarSubRegister::FD_DATAIN6_FINEDELAY     ,33	,24	,4}	,
  {HCCStarSubRegister::FD_DATAIN7_FINEDELAY     ,33	,28	,4}	,
  {HCCStarSubRegister::FD_DATAIN8_FINEDELAY     ,34	,0	,4}	,
  {HCCStarSubRegister::FD_DATAIN9_FINEDELAY     ,34	,4	,4}	,
  {HCCStarSubRegister::FD_DATAIN10_FINEDELAY    ,34	,8	,4}	,
  {HCCStarSubRegister::EPLLICP                  ,35	,0	,4}	,
  {HCCStarSubRegister::EPLLCAP                  ,35	,4	,2}	,
  {HCCStarSubRegister::EPLLRES                  ,35	,8	,4}	,
  {HCCStarSubRegister::EPLLREFFREQ              ,35	,12	,2}	,
  {HCCStarSubRegister::EPLLENABLEPHASE          ,35	,16	,8}	,
  {HCCStarSubRegister::EPLLPHASE320A            ,36	,0	,4}	,
  {HCCStarSubRegister::EPLLPHASE320B            ,36	,4	,4}	,
  {HCCStarSubRegister::EPLLPHASE320C            ,36	,8	,4}	,
  {HCCStarSubRegister::EPLLPHASE160A            ,37	,0	,4}	,
  {HCCStarSubRegister::EPLLPHASE160B            ,37	,8	,4}	,
  {HCCStarSubRegister::EPLLPHASE160C            ,37	,16	,4}	,
  {HCCStarSubRegister::STVCLKOUTCUR             ,38	,0	,3}	,
  {HCCStarSubRegister::STVCLKOUTEN              ,38	,3	,1}	,
  {HCCStarSubRegister::LCBOUTCUR                ,38	,4	,3}	,
  {HCCStarSubRegister::LCBOUTEN                 ,38	,7	,1}	,
  {HCCStarSubRegister::R3L1OUTCUR               ,38	,8	,3}	,
  {HCCStarSubRegister::R3L1OUTEN                ,38	,11	,1}	,
  {HCCStarSubRegister::BCHYBCUR                 ,38	,12	,3}	,
  {HCCStarSubRegister::BCHYBEN                  ,38	,15	,1}	,
  {HCCStarSubRegister::LCBAHYBCUR               ,38	,16	,3}	,
  {HCCStarSubRegister::LCBAHYBEN                ,38	,19	,1}	,
  {HCCStarSubRegister::PRLPHYBCUR               ,38	,20	,3}	,
  {HCCStarSubRegister::PRLPHYBEN                ,38	,23	,1}	,
  {HCCStarSubRegister::RCLKHYBCUR               ,38	,24	,3}	,
  {HCCStarSubRegister::RCLKHYBEN                ,38	,27	,1}	,
  {HCCStarSubRegister::DATA1CUR                 ,39	,0	,3}	,
  {HCCStarSubRegister::DATA1ENPRE               ,39	,3	,1}	,
  {HCCStarSubRegister::DATA1ENABLE              ,39	,4	,1}	,
  {HCCStarSubRegister::DATA1TERM                ,39	,5	,1}	,
  {HCCStarSubRegister::DATACLKCUR               ,39	,16	,3}	,
  {HCCStarSubRegister::DATACLKENPRE             ,39	,19	,1}	,
  {HCCStarSubRegister::DATACLKENABLE            ,39	,20	,1}	,
  {HCCStarSubRegister::ICENABLE                 ,40	,0	,11}	,
  {HCCStarSubRegister::ICTRANSSEL               ,40	,16	,3}	,
  {HCCStarSubRegister::TRIGMODE                 ,41	,0	,1}	,
  {HCCStarSubRegister::ROSPEED                  ,41	,4	,1}	,
  {HCCStarSubRegister::OPMODE                   ,41	,8	,2}	,
  {HCCStarSubRegister::MAXNPACKETS              ,41	,12	,3}	,
  {HCCStarSubRegister::ENCODECNTL               ,41	,16	,1}	,
  {HCCStarSubRegister::ENCODE8B10B              ,41	,17	,1}	,
  {HCCStarSubRegister::PRBSMODE                 ,41	,18	,1}	,
  {HCCStarSubRegister::TRIGMODEC                ,42	,0	,1}	,
  {HCCStarSubRegister::ROSPEEDC                 ,42	,4	,1}	,
  {HCCStarSubRegister::OPMODEC                  ,42	,8	,2}	,
  {HCCStarSubRegister::MAXNPACKETSC             ,42	,12	,3}	,
  {HCCStarSubRegister::ENCODECNTLC              ,42	,16	,1}	,
  {HCCStarSubRegister::ENCODE8B10BC             ,42	,17	,1}	,
  {HCCStarSubRegister::PRBSMODEC                ,42	,18	,1}	,
  {HCCStarSubRegister::BGSETTING                ,43	,0	,5}	,
  {HCCStarSubRegister::MASKHPR                  ,43	,8	,1}	,
  {HCCStarSubRegister::GPO0                     ,43	,12	,1}	,
  {HCCStarSubRegister::GPO1                     ,43	,13	,1}	,
  {HCCStarSubRegister::EFUSEPROGBIT             ,43	,16	,5}	,
  {HCCStarSubRegister::BCIDRSTDELAY             ,44	,0	,9}	,
  {HCCStarSubRegister::BCMMSQUELCH              ,44	,16	,11}	,
  {HCCStarSubRegister::ABCRESETB                ,45	,0	,1}	,
  {HCCStarSubRegister::AMACSSSH                 ,45	,4	,1}	,
  {HCCStarSubRegister::ABCRESETBC               ,46	,0	,1}	,
  {HCCStarSubRegister::AMACSSSHC                ,46	,4	,1}	,
  {HCCStarSubRegister::LCBERRCOUNTTHR           ,47	,0	,16}	,
  {HCCStarSubRegister::R3L1ERRCOUNTTHR          ,47	,16	,16}	,
  {HCCStarSubRegister::AMENABLE                 ,48	,0	,1}	,
  {HCCStarSubRegister::AMCALIB                  ,48	,4	,1}	,
  {HCCStarSubRegister::AMSW0                    ,48	,8	,1}	,
  {HCCStarSubRegister::AMSW1                    ,48	,9	,1}	,
  {HCCStarSubRegister::AMSW2                    ,48	,10	,1}	,
  {HCCStarSubRegister::AMSW60                   ,48	,12	,1}	,
  {HCCStarSubRegister::AMSW80                   ,48	,13	,1}	,
  {HCCStarSubRegister::AMSW100                  ,48	,14	,1}	,
  {HCCStarSubRegister::ANASET                   ,48	,16	,3}	,
  {HCCStarSubRegister::THERMOFFSET              ,48	,20	,4}	
};

HccStarRegInfo::HccStarRegInfo() {
  for (HCCStarRegister reg : HCCStarRegister::_values()) {
    int addr = reg;
    hccregisterMap[addr] = std::make_shared<RegisterInfo>(addr);
  }

  for (auto def : s_hccsubregdefs) {
    auto reg_id = std::get<0>(def);
    std::string subregname = std::string(reg_id._to_string());
    auto addr = std::get<1>(def);
    auto offset = std::get<2>(def);
    auto width = std::get<3>(def);
    hccSubRegisterMap_all[reg_id] = hccregisterMap[addr]->addSubRegister(subregname, offset, width);
  }
}

HccCfg::HccCfg()
  : m_info(HccStarRegInfo::instance())
{
  setupMaps();
  setDefaults();
}

void HccCfg::setupMaps() {
  auto len = HCCStarRegister::_size();
  // In case it's not already empty
  m_registerSet.clear();
  m_registerSet.reserve( len );

  //all HCC Register addresses we will create
  for (HCCStarRegister reg : HCCStarRegister::_values()) {
    int addr = reg;
    Register tmp_Reg(m_info->hccregisterMap[addr], 0);

    m_registerSet.push_back( std::move(tmp_Reg) ); //Save it to the list
    int lastReg = m_registerSet.size()-1;
    m_registerMap[addr]=&m_registerSet.at(lastReg); //Save it's position in memory to the registerMap
  }

  if(m_registerSet.size() != len) {
    logger->info("Mismatch between size {} and values {}", len, m_registerSet.size());
  }
}

void HccCfg::setDefaults() {
  ////  Register* this_Reg = registerMap[0][addr];
  m_registerMap[HCCStarRegister::Pulse]->setValue(0x00000000);
  m_registerMap[HCCStarRegister::Delay1]->setValue(0x00000000);
  m_registerMap[HCCStarRegister::Delay2]->setValue(0x00000000);
  m_registerMap[HCCStarRegister::Delay3]->setValue(0x00000000);
  m_registerMap[HCCStarRegister::PLL1]->setValue(0x00ff3b05);
  m_registerMap[HCCStarRegister::PLL2]->setValue(0x00000000);
  m_registerMap[HCCStarRegister::PLL3]->setValue(0x00000004);
  m_registerMap[HCCStarRegister::DRV1]->setValue(0x00000000);
  m_registerMap[HCCStarRegister::DRV2]->setValue(0x00000014);
  m_registerMap[HCCStarRegister::ICenable]->setValue(0x00000000);
  m_registerMap[HCCStarRegister::OPmode]->setValue(0x00020001);
  m_registerMap[HCCStarRegister::OPmodeC]->setValue(0x00020001);
  m_registerMap[HCCStarRegister::Cfg1]->setValue(0x00000000);
  m_registerMap[HCCStarRegister::Cfg2]->setValue(0x0000018e);
  m_registerMap[HCCStarRegister::ExtRst]->setValue(0x00710003);
  m_registerMap[HCCStarRegister::ExtRstC]->setValue(0x00710003);
  m_registerMap[HCCStarRegister::ErrCfg]->setValue(0x00000000);
  m_registerMap[HCCStarRegister::ADCcfg]->setValue(0x00406600);
}

uint32_t HccCfg::getRegisterValue(HCCStarRegister addr) const {
  return getRegister(addr).getValue();
}

void HccCfg::setRegisterValue(HCCStarRegister addr, uint32_t val) {
  getRegister(addr).setValue(val);
}
