// #################################
// # Project:
// # Description: Star Library
// # Comment: Star Config class
// ################################

#include "StarCfg.h"

StarCfg::StarCfg()  {}

StarCfg::~StarCfg() {}


//Register enums definitions
typedef std::tuple<ABCStarSubRegister, unsigned int, unsigned int, unsigned int> abcsubrefdef;
const std::vector<abcsubrefdef> StarCfg::s_abcsubregdefs = {
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

double StarCfg::toCharge(double vcal) {
    // Q = C*V
    // Linear is good enough
//    double V = (m_vcalPar[0]*Unit::Milli + m_vcalPar[1]*vcal*Unit::Milli)/Physics::ElectronCharge;
//    return V*m_injCap*Unit::Femto;
    return vcal;

}

double StarCfg::toCharge(double vcal, bool sCap, bool lCap) { return toCharge(vcal); }

unsigned StarCfg::toVcal(double charge) {
//    double V= (charge*Physics::ElectronCharge)/(m_injCap*Unit::Femto);
//    unsigned vcal = (unsigned) round((V)/(m_vcalPar[1]*Unit::Milli)); // Note: no offset applied
//    return vcal;
}



void StarCfg::initRegisterMaps() {

  int n_HCC_registers = 50;
  int n_ABC_registers = 128;
  AllReg_List.reserve( n_HCC_registers + m_nABC*n_ABC_registers );

  //Make all registers and subregisters for the HCC
  configure_HCC_Registers();


  //Now make registers for each ABC
  std::cout << "Now have m_nABC as " << m_nABC << std::endl;
  for( int iABC = 0; iABC < m_nABC; ++iABC){ //Start at 1 b/c zero is HCC!
    //Make all registers and subregisters for the HCC
    int this_chipID = getABCchipID(iABC+1);
    configure_ABC_Registers(this_chipID);
  }

}


void StarCfg::configure_HCC_Registers() {

  //all HCC Register addresses we will create
  for (HCCStarRegister reg : HCCStarRegister::_values()) {
    int addr = reg;
    Register tmp_Reg = Register(addr, 0 );

    AllReg_List.push_back( tmp_Reg ); //Save it to the list
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


//TODO ###declare subregisters for HCC

  //    hccSubRegisterMap_all[0]["CFD_BC_FINEDELAY"] = registerMap[0][]->addSubRegister("CFD_BC_FINEDELAY",   ,  );
  //    hccSubRegisterMap_all[0]["CFD_BC_COARSEDELAY"] = registerMap[0][]->addSubRegister("CFD_BC_COARSEDELAY",   ,  );
  //    hccSubRegisterMap_all[0][""] = registerMap[0][]->addSubRegister("",   ,  );
  //    hccSubRegisterMap_all[0][""] = registerMap[0][]->addSubRegister("",   ,  );
  //    hccSubRegisterMap_all[0][""] = registerMap[0][]->addSubRegister("",   ,  );


  //    hccSubRegisterMap_all[0][""] = registerMap[0][]->addSubRegister("",   ,  );






}

void StarCfg::configure_ABC_Registers(int chipID) {

  //DD    //Loop over each ABC register in the default list, and create the Register object
  //DD    //Add the location in memory of this Register to the register maps
  unsigned int chipIndex = indexForABCchipID(chipID);
  for (ABCStarRegister reg : ABCStarRegister::_values()) {
    int addr = reg;
    Register tmp_Reg = Register( addr, 0 );
    AllReg_List.push_back( tmp_Reg ); //Save it to the list
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


void StarCfg::fromFileBinary() {
}

void StarCfg::toFileBinary() {
}

void StarCfg::toFileJson(json &j) {
}

void StarCfg::fromFileJson(json &j) {
}

