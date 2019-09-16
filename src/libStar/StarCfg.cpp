// #################################
// # Project:
// # Description: Star Library
// # Comment: Star Config class
// ################################

#include "StarCfg.h"

StarCfg::StarCfg() {}

StarCfg::~StarCfg() {}


void StarCfg::initRegisterMaps() {

    int n_HCC_registers = 50;
    int n_ABC_registers = 50;
    AllReg_List.reserve( n_HCC_registers + m_nABC*n_ABC_registers );

    //Make all registers and subregisters for the HCC
    configure_HCC_Registers();


    //Now make registers for each ABC
    std::cout << "Now have m_nABC as " << m_nABC << std::endl;
    for( int iABC = 0; iABC < m_nABC; ++iABC){ //Start at 1 b/c zero is HCC!
        //Make all registers and subregisters for the HCC
        int this_chipID = m_chipIDs.at(iABC+1);
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
//    registerMap[0][16]->setValue(0x00000000);
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


//    subRegisterMap_all[0]["CFD_BC_FINEDELAY"] = registerMap[0][]->addSubRegister("CFD_BC_FINEDELAY",   ,  );
//    subRegisterMap_all[0]["CFD_BC_COARSEDELAY"] = registerMap[0][]->addSubRegister("CFD_BC_COARSEDELAY",   ,  );
//    subRegisterMap_all[0][""] = registerMap[0][]->addSubRegister("",   ,  );
//    subRegisterMap_all[0][""] = registerMap[0][]->addSubRegister("",   ,  );
//    subRegisterMap_all[0][""] = registerMap[0][]->addSubRegister("",   ,  );


//    subRegisterMap_all[0][""] = registerMap[0][]->addSubRegister("",   ,  );






}

void StarCfg::configure_ABC_Registers(int chipID) {

    //List of all ABC Register addresses we will create
    std::vector<int> ABC_Register_Addresses = {0,1,2,3,4,6,7,32, 33, 34, 35,36,37,38, 16, 17, 18, 19, 20, 21, 22, 23, 104,105,106,107,108,109,110,111};

//DD    //Loop over each ABC register in the default list, and create the Register object
//DD    //Add the location in memory of this Register to the register map
//DD	std::map<unsigned, Register*> registerMap_ABC;
//DD	std::map<std::string, SubRegister*> subRegisterMap_ABC;

    for (unsigned int iReg = 0; iReg < ABC_Register_Addresses.size(); ++iReg){

        int addr = ABC_Register_Addresses.at(iReg);
        Register tmp_Reg = Register( addr, 0 );
        AllReg_List.push_back( tmp_Reg ); //Save it to the list
        int lastReg = AllReg_List.size()-1;
        registerMap[chipID][addr]=&AllReg_List.at(lastReg); //Save it's position in memory to the registerMap

//        Register* this_Reg = registerMap[chipID][addr];
    }


    //// Initialize 32-bit register with default values
    ////#special reg
    registerMap[chipID][0]->setValue(0x00000000);

    ////#Analog and DCS regs
    registerMap[chipID][1]->setValue(0x00000000);
    registerMap[chipID][2]->setValue(0x00000000);
    registerMap[chipID][3]->setValue(0x00000000);
    registerMap[chipID][4]->setValue(0x00000000);
    registerMap[chipID][6]->setValue(0x00000000);
    registerMap[chipID][7]->setValue(0x00000000);

    ////#Congfiguration regs
    registerMap[chipID][32]->setValue(0x00000000);
    registerMap[chipID][33]->setValue(0x00000000);
    registerMap[chipID][34]->setValue(0x00000000);
	registerMap[chipID][35]->setValue(0x00000000);
	registerMap[chipID][36]->setValue(0x00000000);
	registerMap[chipID][37]->setValue(0x00000000);
	registerMap[chipID][38]->setValue(0x00000000);

	////# Input (Mask) regs
    registerMap[chipID][16]->setValue(0x00000000);
	registerMap[chipID][17]->setValue(0x00000000);
	registerMap[chipID][18]->setValue(0x00000000);
	registerMap[chipID][19]->setValue(0x00000000);
	registerMap[chipID][20]->setValue(0x00000000);
	registerMap[chipID][21]->setValue(0x00000000);
	registerMap[chipID][22]->setValue(0x00000000);
	registerMap[chipID][23]->setValue(0x00000000);


	////# TrimDac regs
//	for(int i=64; i<104;i++)
//		registerMap[chipID][i]->setValue(0x00000000);

	////# Calibration Enable regs
	for(int i=104; i<112;i++)
			registerMap[chipID][i]->setValue(0xFFFFFFFF);




	////#declare subregisters
	////NOTE: If the name is changed here, make sure the corresponding subregister name is also changed in the config json file.

	//subRegister* addSubRegister(std::string subRegName="", unsigned bOffset=0, unsigned mask=0, bool msbRight=false){
			//<subRegName, nthStartBit, nBit,isReversed?>

//	subRegisterMap_all[chipID]["RR_FORCE"] 	   	= registerMap[chipID][0]->addSubRegister("RR_FORCE",    		0, 1);
//	subRegisterMap_all[chipID]["WRITE_DISABLE"] 	= registerMap[chipID][0]->addSubRegister("WRITE_DISABLE",   	1, 1);
//	subRegisterMap_all[chipID]["STOP_HPR"] 		= registerMap[chipID][0]->addSubRegister("STOPHPR",   		2, 1);
//	subRegisterMap_all[chipID]["TEST_HPR"] 		= registerMap[chipID][0]->addSubRegister("TESTHPR",  		3, 1);
//	subRegisterMap_all[chipID]["EFUSEL"] 		= registerMap[chipID][0]->addSubRegister("EFUSEL",  		4, 1);
//	subRegisterMap_all[chipID]["LCB_ERRCNTCLR"] 	= registerMap[chipID][0]->addSubRegister("LCB_ERRCNTCLR",  	5, 1);



	subRegisterMap_all[chipID]["BVREF"] 	= registerMap[chipID][1]->addSubRegister("BVREF",     0,  5);
	subRegisterMap_all[chipID]["BIREF"] 	= registerMap[chipID][1]->addSubRegister("BIREF",     8,  5);
	subRegisterMap_all[chipID]["B8BREF"] 	= registerMap[chipID][1]->addSubRegister("B8BREF",   16,  5);
	subRegisterMap_all[chipID]["BTRANGE"] 	= registerMap[chipID][1]->addSubRegister("BTRANGE",  24,  5);


	subRegisterMap_all[chipID]["BVT"] 		= registerMap[chipID][2]->addSubRegister("BVT",    	  0,  8);
	subRegisterMap_all[chipID]["COMBIAS"] 	= registerMap[chipID][2]->addSubRegister("COMBIAS",   8,  5);
	subRegisterMap_all[chipID]["BIFEED"] 	= registerMap[chipID][2]->addSubRegister("BIFEED",   16,  5);
	subRegisterMap_all[chipID]["BIPRE"] 	= registerMap[chipID][2]->addSubRegister("BIPRE",  	 24,  5);


	subRegisterMap_all[chipID]["STR_DEL_R"] = registerMap[chipID][3]->addSubRegister("STR_DEL_R",    0,  2);
	subRegisterMap_all[chipID]["STR_DEL"] 	= registerMap[chipID][3]->addSubRegister("STR_DEL",    	 8,  6);
	subRegisterMap_all[chipID]["BCAL"] 		= registerMap[chipID][3]->addSubRegister("BCAL",   		16,  9);
	subRegisterMap_all[chipID]["BCAL_RANGE"] = registerMap[chipID][3]->addSubRegister("BCAL_RANGE",   25,  1);

	subRegisterMap_all[chipID]["ADC_BIAS"] 	 = registerMap[chipID][4]->addSubRegister("ADC_BIAS",   0,  4);
	subRegisterMap_all[chipID]["ADC_CH"] 	 = registerMap[chipID][4]->addSubRegister("ADC_CH",   	4,  4);
	subRegisterMap_all[chipID]["ADC_ENABLE"] = registerMap[chipID][4]->addSubRegister("ADC_ENABLE", 8,  1);


	subRegisterMap_all[chipID]["D_S"] 		= registerMap[chipID][6]->addSubRegister("D_S",    		0,  15);
	subRegisterMap_all[chipID]["D_LOW"] 	= registerMap[chipID][6]->addSubRegister("D_LOW",   	15,  1);
	subRegisterMap_all[chipID]["D_EN_CTRL"] = registerMap[chipID][6]->addSubRegister("D_EN_CTRL",   16,  1);


	subRegisterMap_all[chipID]["BTMUX"] 	= registerMap[chipID][7]->addSubRegister("BTMUX",    	0,   14);
	subRegisterMap_all[chipID]["BTMUXD"] 	= registerMap[chipID][7]->addSubRegister("BTMUXD",    	14,   1);
	subRegisterMap_all[chipID]["A_S"] 		= registerMap[chipID][7]->addSubRegister("A_S",    		15,  15);
	subRegisterMap_all[chipID]["A_EN_CTRL"] = registerMap[chipID][7]->addSubRegister("A_EN_CTRL",   31,   1);


	subRegisterMap_all[chipID]["TEST_PULSE_ENABLE"]	= registerMap[chipID][32]->addSubRegister("TEST_PULSE_ENABLE", 4,  1);
	subRegisterMap_all[chipID]["ENCOUNT"] 			= registerMap[chipID][32]->addSubRegister("ENCOUNT",         5,  1);
	subRegisterMap_all[chipID]["MASKHPR"] 			= registerMap[chipID][32]->addSubRegister("MASKHPR",      	 6,  1);
	subRegisterMap_all[chipID]["PR_ENABLE"] 		= registerMap[chipID][32]->addSubRegister("PR_ENABLE",  	 8,  1);
	subRegisterMap_all[chipID]["LP_ENABLE"] 		= registerMap[chipID][32]->addSubRegister("LP_ENABLE",   	 9,  1);
	subRegisterMap_all[chipID]["RRMODE"] 			= registerMap[chipID][32]->addSubRegister("RRMODE",   		10,  2);
	subRegisterMap_all[chipID]["TM"] 				= registerMap[chipID][32]->addSubRegister("TM",  			16,  2);
	subRegisterMap_all[chipID]["TESTPATT_ENABLE"] 	= registerMap[chipID][32]->addSubRegister("TESTPATT_ENABLE", 18,  1);
	subRegisterMap_all[chipID]["TESTPATT1"] 		= registerMap[chipID][32]->addSubRegister("TESTPATT1",      20,  4);
	subRegisterMap_all[chipID]["TESTPATT2"] 		= registerMap[chipID][32]->addSubRegister("TESTPATT2",   	24,  4);


	subRegisterMap_all[chipID]["CURRDRIV"] 			= registerMap[chipID][33]->addSubRegister("CURRDRIV",         0,  3);
	subRegisterMap_all[chipID]["CALPULSE_ENABLE"] 	= registerMap[chipID][33]->addSubRegister("CALPULSE_ENABLE",   4,  1);
	subRegisterMap_all[chipID]["CALPULSE_POLARITY"] 	= registerMap[chipID][33]->addSubRegister("CALPULSE_POLARITY", 5,  1);


	subRegisterMap_all[chipID]["LATENCY"] 		= registerMap[chipID][34]->addSubRegister("LATENCY",     	0,  9);
	subRegisterMap_all[chipID]["BCFLAG_ENABLE"] 	= registerMap[chipID][34]->addSubRegister("BCFLAG_ENABLE",   23, 1);
	subRegisterMap_all[chipID]["BCOFFSET"] 		= registerMap[chipID][34]->addSubRegister("BCOFFSET", 		24, 8);


	subRegisterMap_all[chipID]["DETMODE"] 			 = registerMap[chipID][35]->addSubRegister("DETMODE",  	 0,  2);
	subRegisterMap_all[chipID]["MAX_CLUSTER"] 		 = registerMap[chipID][35]->addSubRegister("MAX_CLUSTER", 12,  6);
	subRegisterMap_all[chipID]["MAX_CLUSTER_ENABLE"] = registerMap[chipID][35]->addSubRegister("MAX_CLUSTER_ENABLE", 	18,  1);


	subRegisterMap_all[chipID]["EN_CLUSTER_EMPTY"] 	= registerMap[chipID][36]->addSubRegister("EN_CLUSTER_EMPTY", 	0, 1);
	subRegisterMap_all[chipID]["EN_CLUSTER_FULL"] 	= registerMap[chipID][36]->addSubRegister("EN_CLUSTER_FULL",	1, 1);
	subRegisterMap_all[chipID]["EN_CLUSTER_OVFL"] 	= registerMap[chipID][36]->addSubRegister("EN_CLUSTER_OVFL",	2, 1);
	subRegisterMap_all[chipID]["EN_REGFIFO_EMPTY"] 	= registerMap[chipID][36]->addSubRegister("EN_REGFIFO_EMPTY",	3, 1);
	subRegisterMap_all[chipID]["EN_REGFIFO_FULL"] 	= registerMap[chipID][36]->addSubRegister("EN_REGFIFO_FULL",	4, 1);
	subRegisterMap_all[chipID]["EN_REGFIFO_OVFL"] 	= registerMap[chipID][36]->addSubRegister("EN_REGFIFO_OVFL",	5, 1);
	subRegisterMap_all[chipID]["EN_LPFIFO_EMPTY"] 	= registerMap[chipID][36]->addSubRegister("EN_LPFIFO_EMPTY",	6, 1);
	subRegisterMap_all[chipID]["EN_LPFIFO_FULL"] 	= registerMap[chipID][36]->addSubRegister("EN_LPFIFO_FULL",		7, 1);
	subRegisterMap_all[chipID]["EN_PRFIFO_EMPTY"] 	= registerMap[chipID][36]->addSubRegister("EN_PRFIFO_EMPTY",	8, 1);
	subRegisterMap_all[chipID]["EN_PRFIFO_FULL"] 	= registerMap[chipID][36]->addSubRegister("EN_PRFIFO_FULL",		9, 1);
	subRegisterMap_all[chipID]["EN_LCB_LOCKED"] 	= registerMap[chipID][36]->addSubRegister("EN_LCB_LOCKED",		10, 1);
	subRegisterMap_all[chipID]["EN_LCB_DECODE_ERR"] = registerMap[chipID][36]->addSubRegister("EN_LCB_DECODE_ERR",	11, 1);
	subRegisterMap_all[chipID]["EN_LCB_ERRCNT_OVFL"]= registerMap[chipID][36]->addSubRegister("EN_LCB_ERRCNT_OVFL",	12, 1);
	subRegisterMap_all[chipID]["EN_LCB_SCMD_ERR"] 	= registerMap[chipID][36]->addSubRegister("EN_LCB_SCMD_ERR",	13, 1);

	subRegisterMap_all[chipID]["DOFUSE"] = registerMap[chipID][37]->addSubRegister("DOFUSE", 0, 24 );

	subRegisterMap_all[chipID]["LCB_ERRCOUNT_THR"] = registerMap[chipID][38]->addSubRegister("LCB_ERRCOUNT_THR", 0, 16);

//	subRegisterMap_all[chipID][""] = registerMap[chipID][36]->addSubRegister("",, );

}



void StarCfg::fromFileBinary() {
}

void StarCfg::toFileBinary() {
}

void StarCfg::toFileJson(json &j) {
}

void StarCfg::fromFileJson(json &j) {
}

