// #################################
// # Project:
// # Description: Star Library
// # Comment: Star Config class
// ################################

#include "StarCfg.h"

#include <iomanip>
#include <iostream>

StarCfg::StarCfg()  {}

StarCfg::~StarCfg() {}


//Register enums definitions
typedef std::tuple<ABCStarSubRegister, unsigned int, unsigned int, unsigned int> abcsubregdef;
const std::vector<abcsubregdef> StarCfg::s_abcsubregdefs = {
  {ABCStarSubRegister::RRFORCE			,0	,0	,1}	,
  {ABCStarSubRegister::WRITEDISABLE		,0	,1	,1}	,
  {ABCStarSubRegister::STOPHPR			,0	,2	,1}	,
  {ABCStarSubRegister::TESTHPR			,0	,3	,1}	,
  {ABCStarSubRegister::EFUSEL			,0	,4	,1}	,
  {ABCStarSubRegister::LCBERRCNTCLR		,0	,5	,1}	,
  {ABCStarSubRegister::BVREF			,1	,0	,5}	,
  {ABCStarSubRegister::BIREF			,1	,8	,5}	,
  {ABCStarSubRegister::B8BREF			,1	,16	,5}	,
  {ABCStarSubRegister::BTRANGE			,1	,24	,5}	,
  {ABCStarSubRegister::BVT			,2	,0	,8}	,
  {ABCStarSubRegister::COMBIAS			,2	,8	,5}	,
  {ABCStarSubRegister::BIFEED			,2	,16	,5}	,
  {ABCStarSubRegister::BIPRE			,2	,24	,5}	,
  {ABCStarSubRegister::STR_DEL_R		,3	,0	,2}	,
  {ABCStarSubRegister::STR_DEL			,3	,8	,6}	,
  {ABCStarSubRegister::BCAL			,3	,16	,9}	,
  {ABCStarSubRegister::BCAL_RANGE		,3	,25	,1}	,
  {ABCStarSubRegister::ADC_BIAS			,4	,0	,4}	,
  {ABCStarSubRegister::ADC_CH			,4	,4	,4}	,
  {ABCStarSubRegister::ADC_ENABLE		,4	,8	,1}	,
  {ABCStarSubRegister::D_S			,6	,0	,1}	,
  {ABCStarSubRegister::D_LOW			,6	,15	,1}	,
  {ABCStarSubRegister::D_EN_CTRL		,6	,16	,1}	,
  {ABCStarSubRegister::BTMUX			,7	,0	,1}	,
  {ABCStarSubRegister::BTMUXD			,7	,14	,1}	,
  {ABCStarSubRegister::A_S			,7	,15	,1}	,
  {ABCStarSubRegister::A_EN_CTRL		,7	,31	,1}	,
  {ABCStarSubRegister::TEST_PULSE_ENABLE	,32	,4	,1}	,
  {ABCStarSubRegister::ENCOUNT			,32	,5	,1}	,
  {ABCStarSubRegister::MASKHPR			,32	,6	,1}	,
  {ABCStarSubRegister::PR_ENABLE		,32	,8	,1}	,
  {ABCStarSubRegister::LP_ENABLE		,32	,9	,1}	,
  {ABCStarSubRegister::RRMODE			,32	,10	,2}	,
  {ABCStarSubRegister::TM			,32	,16	,2}	,
  {ABCStarSubRegister::TESTPATT_ENABLE		,32	,18	,1}	,
  {ABCStarSubRegister::TESTPATT1		,32	,20	,4}	,
  {ABCStarSubRegister::TESTPATT2		,32	,24	,4}	,
  {ABCStarSubRegister::CURRDRIV			,33	,0	,3}	,
  {ABCStarSubRegister::CALPULSE_ENABLE		,33	,4	,1}	,
  {ABCStarSubRegister::CALPULSE_POLARITY	,33	,5	,1}	,
  {ABCStarSubRegister::LATENCY			,34	,0	,9}	,
  {ABCStarSubRegister::BCFLAG_ENABLE		,34	,23	,1}	,
  {ABCStarSubRegister::BCOFFSET			,34	,24	,8}	,
  {ABCStarSubRegister::DETMODE			,35	,0	,2}	,
  {ABCStarSubRegister::MAX_CLUSTER		,35	,12	,6}	,
  {ABCStarSubRegister::MAX_CLUSTER_ENABLE	,35	,18	,1}	,
  {ABCStarSubRegister::EN_CLUSTER_EMPTY		,36	,0	,1}	,
  {ABCStarSubRegister::EN_CLUSTER_FULL		,36	,1	,1}	,
  {ABCStarSubRegister::EN_CLUSTER_OVFL		,36	,2	,1}	,
  {ABCStarSubRegister::EN_REGFIFO_EMPTY		,36	,3	,1}	,
  {ABCStarSubRegister::EN_REGFIFO_FULL		,36	,4	,1}	,
  {ABCStarSubRegister::EN_REGFIFO_OVFL		,36	,5	,1}	,
  {ABCStarSubRegister::EN_LPFIFO_EMPTY		,36	,6	,1}	,
  {ABCStarSubRegister::EN_LPFIFO_FULL		,36	,7	,1}	,
  {ABCStarSubRegister::EN_PRFIFO_EMPTY		,36	,8	,1}	,
  {ABCStarSubRegister::EN_PRFIFO_FULL		,36	,9	,1}	,
  {ABCStarSubRegister::EN_LCB_LOCKED		,36	,10	,1}	,
  {ABCStarSubRegister::EN_LCB_DECODE_ERR	,36	,11	,1}	,
  {ABCStarSubRegister::EN_LCB_ERRCNT_OVFL	,36	,12	,1}	,
  {ABCStarSubRegister::EN_LCB_SCMD_ERR		,36	,13	,1}	,
  {ABCStarSubRegister::DOFUSE			,37	,0	,2}	,
  {ABCStarSubRegister::LCB_ERRCOUNT_THR	        ,38	,0	,1}
};
typedef std::tuple<HCCStarSubRegister, unsigned int, unsigned int, unsigned int> hccsubregdef;
const std::vector<hccsubregdef> StarCfg::s_hccsubregdefs = {
  {HCCStarSubRegister::STOPHPR			,16	,0	,1}	,
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

double StarCfg::toCharge(double vcal) {
    // Q = C*V
    // Linear is good enough
//    double V = (m_vcalPar[0]*Unit::Milli + m_vcalPar[1]*vcal*Unit::Milli)/Physics::ElectronCharge;
//    return V*m_injCap*Unit::Femto;
    return vcal;

}

double StarCfg::toCharge(double vcal, bool sCap, bool lCap) { return toCharge(vcal); }

void StarCfg::initRegisterMaps() {
  auto abc_count = m_ABCchipIDs.size();

  int n_HCC_registers = 50;
  int n_ABC_registers = 128;
  AllReg_List.reserve( n_HCC_registers + abc_count *n_ABC_registers );

  //Make all registers and subregisters for the HCC
  configure_HCC_Registers();


  //Now make registers for each ABC
  std::cout << "Now have m_nABC as " << abc_count << std::endl;
  for( int iABC = 0; iABC < abc_count; ++iABC){ //Start at 1 b/c zero is HCC!
    //Make all registers and subregisters for the HCC
    int this_chipID = getABCchipID(iABC+1);
    configure_ABC_Registers(this_chipID);
  }

}


void StarCfg::configure_HCC_Registers() {

  //all HCC Register addresses we will create
  for (HCCStarRegister reg : HCCStarRegister::_values()) {
    int addr = reg;
    Register tmp_Reg(addr, 0 );

    AllReg_List.push_back( std::move(tmp_Reg) ); //Save it to the list
    int lastReg = AllReg_List.size()-1;
    registerMap[0][addr]=&AllReg_List.at(lastReg); //Save it's position in memory to the registerMap
  }


  ////  Register* this_Reg = registerMap[0][addr];
  registerMap[0][HCCStarRegister::Pulse]->setValue(0x00000000);
  registerMap[0][HCCStarRegister::Delay1]->setValue(0x00000000);
  registerMap[0][HCCStarRegister::Delay2]->setValue(0x00000000);
  registerMap[0][HCCStarRegister::Delay3]->setValue(0x00000000);
  registerMap[0][HCCStarRegister::PLL1]->setValue(0x00ff3b05);
  registerMap[0][HCCStarRegister::PLL2]->setValue(0x00000000);
  registerMap[0][HCCStarRegister::PLL3]->setValue(0x00000004);
  registerMap[0][HCCStarRegister::DRV1]->setValue(0x00000000);
  registerMap[0][HCCStarRegister::DRV2]->setValue(0x00000014);
  registerMap[0][HCCStarRegister::ICenable]->setValue(0x00000000);
  registerMap[0][HCCStarRegister::OPmode]->setValue(0x00020001);
  registerMap[0][HCCStarRegister::OPmodeC]->setValue(0x00020001);
  registerMap[0][HCCStarRegister::Cfg1]->setValue(0x00000000);
  registerMap[0][HCCStarRegister::Cfg2]->setValue(0x0000018e);
  registerMap[0][HCCStarRegister::ExtRst]->setValue(0x00710003);
  registerMap[0][HCCStarRegister::ExtRstC]->setValue(0x00710003);
  registerMap[0][HCCStarRegister::ErrCfg]->setValue(0x00000000);
  registerMap[0][HCCStarRegister::ADCcfg]->setValue(0x00406600);


  for (auto def : s_hccsubregdefs) {
    std::string subregname = std::string((std::get<0>(def))._to_string());
    auto offset = std::get<2>(def);
    auto mask = std::get<3>(def);
    hccSubRegisterMap_all[std::get<0>(def)] = registerMap[0][std::get<1>(def)]->addSubRegister(subregname, offset, mask);
  }





}

void StarCfg::configure_ABC_Registers(int chipID) {

  //DD    //Loop over each ABC register in the default list, and create the Register object
  //DD    //Add the location in memory of this Register to the register maps
  unsigned int chipIndex = indexForABCchipID(chipID);
  for (ABCStarRegister reg : ABCStarRegister::_values()) {
    int addr = reg;
    Register tmp_Reg( addr, 0 );
    AllReg_List.push_back( std::move(tmp_Reg) ); //Save it to the list
    int lastReg = AllReg_List.size()-1;
    registerMap[chipIndex][addr]=&AllReg_List.at(lastReg); //Save it's position in memory to the registerMap
  }


  //// Initialize 32-bit register with default values
  ////#special reg
  registerMap[chipIndex][ABCStarRegister::SCReg]->setValue(0x00000004);

  ////#Analog and DCS regs
  for (unsigned int iReg=ABCStarRegister::ADCS1; iReg<=ABCStarRegister::ADCS7; iReg++)
    registerMap[chipIndex][iReg]->setValue(0x00000000);

  ////#Congfiguration regs
  for (unsigned int iReg=ABCStarRegister::CREG0; iReg<=ABCStarRegister::CREG6; iReg++)
    registerMap[chipIndex][iReg]->setValue(0x00000000);

  ////# Input (Mask) regs
  for (unsigned int iReg=ABCStarRegister::MaskInput0; iReg<=ABCStarRegister::MaskInput7; iReg++)
    registerMap[chipIndex][iReg]->setValue(0x00000000);

  ////# Calibration Enable regs
  for (unsigned int iReg=ABCStarRegister::CalREG0; iReg<=ABCStarRegister::CalREG7; iReg++)
    registerMap[chipIndex][iReg]->setValue(0xFFFFFFFF);


  for (auto def : s_abcsubregdefs) {
    std::string subregname = std::string((std::get<0>(def))._to_string());
    abcSubRegisterMap_all[chipIndex][std::get<0>(def)] = registerMap[chipIndex][std::get<1>(def)]->addSubRegister(subregname, std::get<2>(def), std::get<3>(def));
  }

  /// TODO not sure if this is a good implementation; to-be-optimized.
  /// registerMap is quite large if it includes trimdac, that's 10chips x (256+256) = 5120


  ////# 256 TrimDac regs 4-bit lsb
  int channel=0;

  for(int i=ABCStarRegister::TrimDAC0; i<=ABCStarRegister::TrimDAC31;i++){
  	registerMap[chipIndex][i]->setValue(0xFFFFFFFF);

  	int nthStartBit = 0;
  	for(int j=0; j<8;j++){

  		std::string trimDAC_name = "trimdac_4lsb_"+std::to_string(channel);
  				////std::to_string(((channel>>7)&1)+1)+"_"+std::to_string((channel&0x7f)+1); //trimdac_4lsb_<nthRow>_<nthCol>; row(1-2); col(1-256); match to histogram
//  		std::cout << " reg[" << i <<"] for channel[" << channel  << "]----->" << trimDAC_name <<  "     @ nthStartBit: "<< nthStartBit<< std::endl;
  		trimDAC_4LSB_RegisterMap_all[chipIndex][trimDAC_name] = registerMap[chipIndex][i]->addSubRegister(trimDAC_name, nthStartBit, 4);
  		channel++;
  		nthStartBit+=4;
  	}
  }

  ////# 256 TrimDac regs 1-bit msb
  channel = 0;
  for(int i=ABCStarRegister::TrimDAC32; i<=ABCStarRegister::TrimDAC39;i++){
   	registerMap[chipIndex][i]->setValue(0x00000000);
   	for(int j=0; j<32;j++){
   		std::string trimDAC_name = "trimdac_1msb_"+std::to_string(channel);
   				////std::to_string(((channel>>7)&1)+1)+"_"+std::to_string((channel&0x7f)+1); //trimdac_1msb_<nthRow>_<nthCol>; row(1-2); col(1-256); match to histogram
//   		std::cout << " reg[" << i <<"] for channel[" << channel  << "]----->" << trimDAC_name << std::endl;;
   		trimDAC_1MSB_RegisterMap_all[chipIndex][trimDAC_name] = registerMap[chipIndex][i]->addSubRegister(trimDAC_name, j, 1);
   		channel++;
   	}
   }


}

//HCC register accessor functions
const uint32_t StarCfg::getHCCRegister(uint32_t addr){
  return registerMap[0][addr]->getValue();
}
void StarCfg::setHCCRegister(uint32_t addr, uint32_t val){
  registerMap[0][addr]->setValue(val);
}
//ABC register accessor functions, converts chipID into chip index
const uint32_t StarCfg::getABCRegister(uint32_t addr, int32_t chipID){
  unsigned int index = indexForABCchipID(chipID);
  return registerMap[index][addr]->getValue();
}
void StarCfg::setABCRegister(uint32_t addr, uint32_t val, int32_t chipID){
  unsigned int index = indexForABCchipID(chipID);
  registerMap[index][addr]->setValue(val);
}

void StarCfg::setTrimDAC(unsigned col, unsigned row, int value)  {
	////NOTE: Each chip is divided in 2 row x 128 col. Histogram bins are adjusted based on number of activated chips. Does not have gap in between rows.
	////      Let's say, of the 10 ABC in one hybrid, only chip 0, 4 and 6 are activated, the histogram has 6 rows x 128 cols.
	////      i.e row 1&2 belong to chip_0; row 3&4 belong to chip_4;  row 5&6 belong to chip_6.
	////      the trimDAC_4lsb_name for each chip is trimdac_4lsb_<nthRow[2:1]>_<nthCol[128:1]>
	////      the trimDAC_1msb_name for each chip is trimdac_1msb_<nthRow[2:1]>_<nthCol[128:1]>

	////NOTE: row and col pass from histogram starts from 1, while channel starts from 0

	int nthRow = row%2 ==0 ? 2 : 1;
//	std::string trimDAC_4lsb_name = "trimdac_4lsb_"+std::to_string(nthRow)+"_"+std::to_string(col);
//	std::string trimDAC_1msb_name = "trimdac_1msb_"+std::to_string(nthRow)+"_"+std::to_string(col);



	int channel=0;
	int chn_tmp = floor((col-1)/2);
	if(nthRow==1) channel = (col-1) + chn_tmp*2;
	else if(nthRow==2) channel = (col-1) + (chn_tmp+1)*2;

	std::string trimDAC_4lsb_name = "trimdac_4lsb_"+std::to_string(channel);
	std::string trimDAC_1msb_name = "trimdac_1msb_"+std::to_string(channel);

//std::cout <<  __PRETTY_FUNCTION__ << "    row:" << row-1 << " col:" << (col-1) << " chn_tmp:" <<   chn_tmp << "  channel: " << channel << std::endl;

	unsigned chipIndex = ceil(row/2.0);
	if (trimDAC_4LSB_RegisterMap_all[chipIndex].find(trimDAC_4lsb_name) != trimDAC_4LSB_RegisterMap_all[chipIndex].end()) {
		trimDAC_4LSB_RegisterMap_all[chipIndex][trimDAC_4lsb_name]->updateValue(value&0xf);
	} else {
		std::cerr << " StarCfg::setTrimDAC--> Error: Could not find sub register \""<< trimDAC_4lsb_name << "\" in trimDAC_4LSB_RegisterMap_all for chip[" << chipIndex <<"]" << std::endl;
	}

	if (trimDAC_1MSB_RegisterMap_all[chipIndex].find(trimDAC_1msb_name) != trimDAC_1MSB_RegisterMap_all[chipIndex].end()) {
//		std::cout << " value: " << value << "  " << ((value>>4)&0x1) << std::endl;
		trimDAC_1MSB_RegisterMap_all[chipIndex][trimDAC_1msb_name]->updateValue((value>>4)&0x1);
	} else {
		std::cerr << " StarCfg::setTrimDAC--> Error: Could not find sub register \""<< trimDAC_1msb_name << "\" in trimDAC_1MSB_RegisterMap_all for chip[" << chipIndex <<"]" << std::endl;
	}

}



int StarCfg::getTrimDAC(unsigned col, unsigned row) {

	int nthRow = row%2 ==0 ? 2 : 1;
//	std::string trimDAC_4lsb_name = "trimdac_4lsb_"+std::to_string(nthRow)+"_"+std::to_string(col);
//	std::string trimDAC_1msb_name = "trimdac_1msb_"+std::to_string(nthRow)+"_"+std::to_string(col);

	int channel=0;
	int chn_tmp = floor((col-1)/2);
	if(nthRow==1) channel = (col-1) + chn_tmp*2;
	else if(nthRow==2) channel = (col-1) + (chn_tmp+1)*2;

	std::string trimDAC_4lsb_name = "trimdac_4lsb_"+std::to_string(channel);
	std::string trimDAC_1msb_name = "trimdac_1msb_"+std::to_string(channel);

//std::cout << __PRETTY_FUNCTION__ << "    row:" << row-1 << " col:" << (col-1) << " chn_tmp:" <<   chn_tmp << "  channel: " << channel << std::endl;

	unsigned chipIndex = ceil(row/2.0);



	if (trimDAC_4LSB_RegisterMap_all[chipIndex].find(trimDAC_4lsb_name) == trimDAC_4LSB_RegisterMap_all[chipIndex].end()) {
		std::cerr << " StarCfg::getTrimDAC--> Error: Could not find sub register \""<< trimDAC_4lsb_name << "\" in trimDAC_4LSB_RegisterMap_all for chip[" << chipIndex <<"]" << std::endl;
		return 0;
	}

	if (trimDAC_1MSB_RegisterMap_all[chipIndex].find(trimDAC_1msb_name) == trimDAC_1MSB_RegisterMap_all[chipIndex].end()) {
		std::cerr << " StarCfg::getTrimDAC--> Error: Could not find sub register \""<< trimDAC_1msb_name << "\" in trimDAC_1MSB_RegisterMap_all for chip[" << chipIndex <<"]" << std::endl;
		return 0;
	}

	unsigned trimDAC_4LSB = trimDAC_4LSB_RegisterMap_all[chipIndex][trimDAC_4lsb_name]->getValue();
	unsigned trimDAC_1MSB = trimDAC_1MSB_RegisterMap_all[chipIndex][trimDAC_1msb_name]->getValue();


	if(trimDAC_4LSB > 15 )
		std::cerr << " --> Error: Sub register \""<< trimDAC_4lsb_name << "\" in trimDAC_4LSB_RegisterMap_all for chip[" << chipIndex <<"] is larger than 15 with value" << trimDAC_4LSB << std::endl;

	if(trimDAC_1MSB > 1 )
		std::cerr << " --> Error: Sub register \""<< trimDAC_1msb_name << "\" in trimDAC_1MSB_RegisterMap_all for chip[" << chipIndex <<"] is larger than 1 with value" << trimDAC_1MSB << std::endl;


	return ( (trimDAC_1MSB<<4) | trimDAC_4LSB);


}


void StarCfg::toFileJson(json &j) {
}

void StarCfg::fromFileJson(json &j) {
}
