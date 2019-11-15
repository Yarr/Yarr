// #################################
// # Project:
// # Description: Star Library
// # Comment: Star Config class
// ################################

#include "StarCfg.h"

StarCfg::StarCfg() { defineSubRegisters(); }

StarCfg::~StarCfg() {}

typedef std::tuple<ABCStarSubRegister, unsigned int, unsigned int, unsigned int> abcsubrefdef;
std::vector<abcsubrefdef> StarCfg::s_abcsubregdefs = std::vector<abcsubrefdef>();

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



void StarCfg::defineSubRegisters() {
   if (s_abcsubregdefs.size()) return;
   s_abcsubregdefs.push_back(std::make_tuple(ABCStarSubRegister::BVREF			,1	,0	,5));
   s_abcsubregdefs.push_back(std::make_tuple(ABCStarSubRegister::BIREF			,1	,8	,5));
   s_abcsubregdefs.push_back(std::make_tuple(ABCStarSubRegister::B8BREF			,1	,16	,5));
   s_abcsubregdefs.push_back(std::make_tuple(ABCStarSubRegister::BTRANGE		,1	,24	,5));
   s_abcsubregdefs.push_back(std::make_tuple(ABCStarSubRegister::BVT			,2	,0	,8));
   s_abcsubregdefs.push_back(std::make_tuple(ABCStarSubRegister::COMBIAS		,2	,8	,5));
   s_abcsubregdefs.push_back(std::make_tuple(ABCStarSubRegister::BIFEED			,2	,16	,5));
   s_abcsubregdefs.push_back(std::make_tuple(ABCStarSubRegister::BIPRE			,2	,24	,5));
   s_abcsubregdefs.push_back(std::make_tuple(ABCStarSubRegister::STR_DEL_R		,3	,0	,2));
   s_abcsubregdefs.push_back(std::make_tuple(ABCStarSubRegister::STR_DEL		,3	,8	,6));
   s_abcsubregdefs.push_back(std::make_tuple(ABCStarSubRegister::BCAL			,3	,16	,9));
   s_abcsubregdefs.push_back(std::make_tuple(ABCStarSubRegister::BCAL_RANGE		,3	,25	,1));
   s_abcsubregdefs.push_back(std::make_tuple(ABCStarSubRegister::ADC_BIAS		,4	,0	,4));
   s_abcsubregdefs.push_back(std::make_tuple(ABCStarSubRegister::ADC_CH			,4	,4	,4));
   s_abcsubregdefs.push_back(std::make_tuple(ABCStarSubRegister::ADC_ENABLE		,4	,8	,1));
   s_abcsubregdefs.push_back(std::make_tuple(ABCStarSubRegister::D_S			,6	,0	,15));
   s_abcsubregdefs.push_back(std::make_tuple(ABCStarSubRegister::D_LOW			,6	,15	,1));
   s_abcsubregdefs.push_back(std::make_tuple(ABCStarSubRegister::D_EN_CTRL		,6	,16	,1));
   s_abcsubregdefs.push_back(std::make_tuple(ABCStarSubRegister::BTMUX			,7	,0	,14));
   s_abcsubregdefs.push_back(std::make_tuple(ABCStarSubRegister::BTMUXD			,7	,14	,1));
   s_abcsubregdefs.push_back(std::make_tuple(ABCStarSubRegister::A_S			,7	,15	,15));
   s_abcsubregdefs.push_back(std::make_tuple(ABCStarSubRegister::A_EN_CTRL		,7	,31	,1));
   s_abcsubregdefs.push_back(std::make_tuple(ABCStarSubRegister::TEST_PULSE_ENABLE	,32	,4	,1));
   s_abcsubregdefs.push_back(std::make_tuple(ABCStarSubRegister::ENCOUNT		,32	,5	,1));
   s_abcsubregdefs.push_back(std::make_tuple(ABCStarSubRegister::MASKHPR		,32	,6	,1));
   s_abcsubregdefs.push_back(std::make_tuple(ABCStarSubRegister::PR_ENABLE		,32	,8	,1));
   s_abcsubregdefs.push_back(std::make_tuple(ABCStarSubRegister::LP_ENABLE		,32	,9	,1));
   s_abcsubregdefs.push_back(std::make_tuple(ABCStarSubRegister::RRMODE			,32	,10	,2));
   s_abcsubregdefs.push_back(std::make_tuple(ABCStarSubRegister::TM			,32	,16	,2));
   s_abcsubregdefs.push_back(std::make_tuple(ABCStarSubRegister::TESTPATT_ENABLE	,32	,18	,1));
   s_abcsubregdefs.push_back(std::make_tuple(ABCStarSubRegister::TESTPATT1		,32	,20	,4));
   s_abcsubregdefs.push_back(std::make_tuple(ABCStarSubRegister::TESTPATT2		,32	,24	,4));
   s_abcsubregdefs.push_back(std::make_tuple(ABCStarSubRegister::CURRDRIV		,33	,0	,3));
   s_abcsubregdefs.push_back(std::make_tuple(ABCStarSubRegister::CALPULSE_ENABLE	,33	,4	,1));
   s_abcsubregdefs.push_back(std::make_tuple(ABCStarSubRegister::CALPULSE_POLARITY	,33	,5	,1));
   s_abcsubregdefs.push_back(std::make_tuple(ABCStarSubRegister::LATENCY		,34	,0	,9));
   s_abcsubregdefs.push_back(std::make_tuple(ABCStarSubRegister::BCFLAG_ENABLE		,34	,23	,1));
   s_abcsubregdefs.push_back(std::make_tuple(ABCStarSubRegister::BCOFFSET		,34	,24	,8));
   s_abcsubregdefs.push_back(std::make_tuple(ABCStarSubRegister::DETMODE		,35	,0	,2));
   s_abcsubregdefs.push_back(std::make_tuple(ABCStarSubRegister::MAX_CLUSTER		,35	,12	,6));
   s_abcsubregdefs.push_back(std::make_tuple(ABCStarSubRegister::MAX_CLUSTER_ENABLE	,35	,18	,1));
   s_abcsubregdefs.push_back(std::make_tuple(ABCStarSubRegister::EN_CLUSTER_EMPTY	,36	,0	,1));
   s_abcsubregdefs.push_back(std::make_tuple(ABCStarSubRegister::EN_CLUSTER_FULL	,36	,1	,1));
   s_abcsubregdefs.push_back(std::make_tuple(ABCStarSubRegister::EN_CLUSTER_OVFL	,36	,2	,1));
   s_abcsubregdefs.push_back(std::make_tuple(ABCStarSubRegister::EN_REGFIFO_EMPTY	,36	,3	,1));
   s_abcsubregdefs.push_back(std::make_tuple(ABCStarSubRegister::EN_REGFIFO_FULL	,36	,4	,1));
   s_abcsubregdefs.push_back(std::make_tuple(ABCStarSubRegister::EN_REGFIFO_OVFL	,36	,5	,1));
   s_abcsubregdefs.push_back(std::make_tuple(ABCStarSubRegister::EN_LPFIFO_EMPTY	,36	,6	,1));
   s_abcsubregdefs.push_back(std::make_tuple(ABCStarSubRegister::EN_LPFIFO_FULL		,36	,7	,1));
   s_abcsubregdefs.push_back(std::make_tuple(ABCStarSubRegister::EN_PRFIFO_EMPTY	,36	,8	,1));
   s_abcsubregdefs.push_back(std::make_tuple(ABCStarSubRegister::EN_PRFIFO_FULL		,36	,9	,1));
   s_abcsubregdefs.push_back(std::make_tuple(ABCStarSubRegister::EN_LCB_LOCKED		,36	,10	,1));
   s_abcsubregdefs.push_back(std::make_tuple(ABCStarSubRegister::EN_LCB_DECODE_ERR	,36	,11	,1));
   s_abcsubregdefs.push_back(std::make_tuple(ABCStarSubRegister::EN_LCB_ERRCNT_OVFL	,36	,12	,1));
   s_abcsubregdefs.push_back(std::make_tuple(ABCStarSubRegister::EN_LCB_SCMD_ERR	,36	,13	,1));
   s_abcsubregdefs.push_back(std::make_tuple(ABCStarSubRegister::DOFUSE			,37	,0	,24));
   s_abcsubregdefs.push_back(std::make_tuple(ABCStarSubRegister::LCB_ERRCOUNT_THR	,38	,0	,16) );
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

  //List of all HCC Register addresses we will create
  std::vector<int> HCC_Register_Addresses = {16, 32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48};

  for (unsigned int iReg = 0; iReg < HCC_Register_Addresses.size(); ++iReg){

    int addr = HCC_Register_Addresses.at(iReg);
    Register tmp_Reg = Register(addr, 0 );

    AllReg_List.push_back( tmp_Reg ); //Save it to the list
    int lastReg = AllReg_List.size()-1;
    registerMap[0][addr]=&AllReg_List.at(lastReg); //Save it's position in memory to the registerMap
  }


  ////  Register* this_Reg = registerMap[0][addr];
  registerMap[0][16]->setValue(0x00000000);
  registerMap[0][32]->setValue(0x00000000);
  registerMap[0][33]->setValue(0x00000000);
  registerMap[0][34]->setValue(0x00000000);
  registerMap[0][35]->setValue(0x00ff3b05);
  registerMap[0][36]->setValue(0x00000000);
  registerMap[0][37]->setValue(0x00000004);
  registerMap[0][38]->setValue(0x00000000);
  registerMap[0][39]->setValue(0x00000014);
  registerMap[0][40]->setValue(0x00000000);
  registerMap[0][41]->setValue(0x00020001);
  registerMap[0][42]->setValue(0x00020001);
  registerMap[0][43]->setValue(0x00000000);
  registerMap[0][44]->setValue(0x0000018e);
  registerMap[0][45]->setValue(0x00710003);
  registerMap[0][46]->setValue(0x00710003);
  registerMap[0][47]->setValue(0x00000000);
  registerMap[0][48]->setValue(0x00406600);


//TODO ###declare subregisters for HCC

  //    hccSubRegisterMap_all[0]["CFD_BC_FINEDELAY"] = registerMap[0][]->addSubRegister("CFD_BC_FINEDELAY",   ,  );
  //    hccSubRegisterMap_all[0]["CFD_BC_COARSEDELAY"] = registerMap[0][]->addSubRegister("CFD_BC_COARSEDELAY",   ,  );
  //    hccSubRegisterMap_all[0][""] = registerMap[0][]->addSubRegister("",   ,  );
  //    hccSubRegisterMap_all[0][""] = registerMap[0][]->addSubRegister("",   ,  );
  //    hccSubRegisterMap_all[0][""] = registerMap[0][]->addSubRegister("",   ,  );


  //    hccSubRegisterMap_all[0][""] = registerMap[0][]->addSubRegister("",   ,  );






}

void StarCfg::configure_ABC_Registers(int chipID) {

  //List of all ABC Register addresses we will create
  std::vector<int> ABC_Register_Addresses = {0,1,2,3,4,6,7,32, 33, 34, 35,36,37,38, 16, 17, 18, 19, 20, 21, 22, 23, 104,105,106,107,108,109,110,111,
		  64, 65, 66, 67, 68, 69, 70, 71, 72, 73, 74, 75, 76, 77, 78, 79, 80, 81, 82, 83, 84, 85, 86, 87, 88, 89, 90, 91, 92, 93, 94, 95, 96, 97, 98, 99, 100, 101, 102, 103, 104};

  //DD    //Loop over each ABC register in the default list, and create the Register object
  //DD    //Add the location in memory of this Register to the register map
  //DD	std::map<unsigned, Register*> registerMap_ABC;
  //DD	std::map<std::string, SubRegister*> subRegisterMap_ABC;

  unsigned int chipIndex = indexForABCchipID(chipID);
  for (unsigned int iReg = 0; iReg < ABC_Register_Addresses.size(); ++iReg){

    int addr = ABC_Register_Addresses.at(iReg);
    Register tmp_Reg = Register( addr, 0 );
    AllReg_List.push_back( tmp_Reg ); //Save it to the list
    int lastReg = AllReg_List.size()-1;
    registerMap[chipIndex][addr]=&AllReg_List.at(lastReg); //Save it's position in memory to the registerMap

    //        Register* this_Reg = registerMap[chipID][addr];
  }


  //// Initialize 32-bit register with default values
  ////#special reg
  registerMap[chipIndex][0]->setValue(0x00000004);

  ////#Analog and DCS regs
  registerMap[chipIndex][1]->setValue(0x00000000);
  registerMap[chipIndex][2]->setValue(0x00000000);
  registerMap[chipIndex][3]->setValue(0x00000000);
  registerMap[chipIndex][4]->setValue(0x00000000);
  registerMap[chipIndex][6]->setValue(0x00000000);
  registerMap[chipIndex][7]->setValue(0x00000000);

  ////#Congfiguration regs
  registerMap[chipIndex][32]->setValue(0x00000000);
  registerMap[chipIndex][33]->setValue(0x00000000);
  registerMap[chipIndex][34]->setValue(0x00000000);
  registerMap[chipIndex][35]->setValue(0x00000000);
  registerMap[chipIndex][36]->setValue(0x00000000);
  registerMap[chipIndex][37]->setValue(0x00000000);
  registerMap[chipIndex][38]->setValue(0x00000000);

  ////# Input (Mask) regs
  registerMap[chipIndex][16]->setValue(0x00000000);
  registerMap[chipIndex][17]->setValue(0x00000000);
  registerMap[chipIndex][18]->setValue(0x00000000);
  registerMap[chipIndex][19]->setValue(0x00000000);
  registerMap[chipIndex][20]->setValue(0x00000000);
  registerMap[chipIndex][21]->setValue(0x00000000);
  registerMap[chipIndex][22]->setValue(0x00000000);
  registerMap[chipIndex][23]->setValue(0x00000000);


  ////# Calibration Enable regs
  for(int i=104; i<112;i++)
    registerMap[chipIndex][i]->setValue(0xFFFFFFFF);



  for (auto def : s_abcsubregdefs) {
    std::string subregname = std::string((std::get<0>(def))._to_string());
    abcSubRegisterMap_all[chipIndex][std::get<0>(def)] = registerMap[chipIndex][std::get<1>(def)]->addSubRegister(subregname, std::get<2>(def), std::get<3>(def));
  }

  /// TODO not sure if this is a good implementation; to-be-optimized.
  /// registerMap is quite large if it includes trimdac, that's 10chips x (256+256) = 5120


  ////# 256 TrimDac regs 4-bit lsb
  int channel=0;

  for(int i=64; i<96;i++){
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
  for(int i=96; i<104;i++){
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

