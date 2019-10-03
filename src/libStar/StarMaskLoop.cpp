#include "StarMaskLoop.h"
#include "StarMask_CalEn.h"

StarMaskLoop::StarMaskLoop() : LoopActionBase() {
    m_mask = MASK_16;
    min = 0;
    max = 16;
    step = 1;
    m_cur = 0;
    enable_sCap = false;
    enable_lCap = false;
    loopType = typeid(this);
    verbose=false;
}


void StarMaskLoop::init() {
	//printMask(0,12,2,0);
//	std::cout << std::endl;
//	std::cout << "calEN" << std::endl;
//	printMask(0, 5,1,3);
    m_done = false;
    if (verbose)
        std::cout << __PRETTY_FUNCTION__ << std::endl;
//    // Shift Mask into all pixels
//    keeper->globalFe<Star>()->writeRegister(&Star::Colpr_Mode, 0x3);
//    keeper->globalFe<Star>()->writeRegister(&Star::Colpr_Addr, 0x0);
//    keeper->globalFe<Star>()->initMask(MASK_1);
//    if (enable_lCap) keeper->globalFe<Star>()->loadIntoPixel(1 << 6);
//    if (enable_sCap) keeper->globalFe<Star>()->loadIntoPixel(1 << 7);
//    keeper->globalFe<Star>()->initMask(m_mask);
//    keeper->globalFe<Star>()->loadIntoPixel(1 << 0);
    m_cur = min;
    for ( FrontEnd* fe : keeper->feList ) {
//    	if (dynamic_cast<FrontEndCfg*>(fe)->getName().find("hcc") != std::string::npos) {continue;}  ///fix me, maybe @@@
    	if (!fe->isActive()) {continue;}

    	for(int iChip = 1; iChip < static_cast<StarChips*> (fe)->m_nABC+1; ++iChip){ //exclude iChip=0 which is the Hcc

//    		std::cout << "static_cast<Hcc*> (fe)->m_nABC: " << static_cast<Hcc*> (fe)->m_nABC << "  " << iChip << " ichip with id: " << this_chipID << std::endl;
//    		    	std::cout << "Star_masks[" << m_cur << "]: " << std::hex <<  Star_masks[m_cur] <<std::dec << std::endl;
//    		    	std::cout << "Star_calEn[" << m_cur << "]: " << std::hex <<  Star_calEn[m_cur] << std::dec<< std::endl;
    		int index =0;
    		for (int i=16; i<24; i++){
//    			std::cout << "write mask: "<< i << "  0x" << std::hex << std::setfill('0') << std::setw(8)<< Star_masks[min][index] <<std::dec<< std::endl;
    			static_cast<StarChips*> (fe)->setAndWriteABCRegister(i, star_masks[min][index], iChip );  // strip 0's mask starts from reg 16,
    			index++;
    		}
    		index =0;
    		for (int i=104; i<112; i++){
//    			std::cout << "write cal: "<< i  << " 0x" << std::hex << std::setfill('0') << std::setw(8)<< Star_calEn[min][index] << std::dec<< std::endl;
    			static_cast<StarChips*> (fe)->setAndWriteABCRegister(i, star_calEn[min][index], iChip);  // strip 0's mask starts from reg 16,
    			index++;
    		}
    	}
    }


    while(g_tx->isCmdEmpty() == 0);
}

void StarMaskLoop::end() {
    if (verbose)
        std::cout << __PRETTY_FUNCTION__ << std::endl;
//    // Disable all pixels
//    keeper->globalFe<Star>()->writeRegister(&Star::Colpr_Mode, 0x3);
//    keeper->globalFe<Star>()->writeRegister(&Star::Colpr_Addr, 0x0);
//    keeper->globalFe<Star>()->initMask(MASK_NONE);
//    keeper->globalFe<Star>()->loadIntoPixel(1 << 0);
//    if (enable_lCap) keeper->globalFe<Star>()->loadIntoPixel(1 << 6);
//    if (enable_sCap) keeper->globalFe<Star>()->loadIntoPixel(1 << 7);
    while(g_tx->isCmdEmpty() == 0);
}

void StarMaskLoop::execPart1() {
    if (verbose)
        std::cout << __PRETTY_FUNCTION__ << std::endl;

//    std::cout << " ---> Mask Stage " << m_cur << std::endl;
    g_stat->set(this, m_cur);
}

void StarMaskLoop::execPart2() {
    if (verbose)
        std::cout << __PRETTY_FUNCTION__ << std::endl;
    m_cur += step;
    if (!((int)m_cur < max)) m_done = true;
    else{
    // Shift Enable mask by step size
    for(unsigned i=0; i<step; i++) {
//        keeper->globalFe<Star>()->writeRegister(&Star::Colpr_Mode, 0x3);
//        keeper->globalFe<Star>()->writeRegister(&Star::Colpr_Addr, 0x0);
//        keeper->globalFe<Star>()->shiftMask();
//        keeper->globalFe<Star>()->loadIntoPixel(1 << 0);
        //if (enable_lCap) keeper->globalFe<Star>()->loadIntoPixel(1 << 6);
        //if (enable_sCap) keeper->globalFe<Star>()->loadIntoPixel(1 << 7);


    	 for ( FrontEnd* fe : keeper->feList ) {
//    		 if (dynamic_cast<FrontEndCfg*>(fe)->getName().find("hcc") != std::string::npos) {continue;}  ///fix me, maybe @@@
    		 if (!fe->isActive()) {continue;}
//    		 std::cout << "\033[1;32m " << "TxCore " << dynamic_cast<FrontEndCfg*>(fe)->getTxChannel()<<"\033[0m"<<std::endl;
    		 //static_cast<Star*> (fe)->setRunMode();
    		 for( int iChip = 1; iChip < static_cast<StarChips*> (fe)->m_nABC+1; ++iChip){ //exclude iChip=0 which is the Hcc
//    			     	 std::cout << "Star_masks[" << m_cur << "]: " << std::hex <<  Star_masks[m_cur] <<std::dec << std::endl;
//    			     	 std::cout << "Star_calEn[" << m_cur << "]: " << std::hex <<  Star_calEn[m_cur] << std::dec<< std::endl;
//    	    	 std::cout << "static_cast<Hcc*> (fe)->m_nABC: " << static_cast<Hcc*> (fe)->m_nABC << "  " << iChip << " ichip with id: " << this_chipID << std::endl;

    			 int index=0;
    			 for (int j=16; j<24; j++){
//    				 std::cout << "write mask: "<< i << "  0x" << std::hex << std::setfill('0') << std::setw(8)<< Star_masks[m_cur][index] <<std::dec<< std::endl;
    				 static_cast<StarChips*> (fe)->setAndWriteABCRegister(j, star_masks[m_cur][index], iChip);  // strip 0's mask starts from reg 16,
    				 index++;
    			 }
    			 index=0;
    			 for (int j=104; j<112; j++){
//    				 std::cout << "write cal: "<< i  << " 0x" << std::hex << std::setfill('0') << std::setw(8)<< Star_calEn[m_cur][index] <<std::dec<< std::endl;
    				 static_cast<StarChips*> (fe)->setAndWriteABCRegister(j, star_calEn[m_cur][index], iChip);  // strip 0's mask starts from reg 104,.
    				 index++;
    			 }

    		 }
    	 }



        while(g_tx->isCmdEmpty() == 0){std::cout << __PRETTY_FUNCTION__ << "\033[1;32m " << "cmd not empty: " << "\033[0m"<<  std::endl;};
    }
    }
}

void StarMaskLoop::setMaskStage(enum MASK_STAGE mask) {
    switch (mask) {
        case MASK_1: //all are masked
            m_mask = MASK_1;
            max = 1;
            break;
        case MASK_2: //every second strip are masked
            m_mask = MASK_2;
            max = 2;
            break;
        case MASK_4: //every forth strip are masked
            m_mask = MASK_4;
            max = 4;
            break;
        case MASK_8:
            m_mask = MASK_8;
            max = 8;
            break;
        case MASK_16:
            m_mask = MASK_16;
            max = 16;
            break;
        case MASK_32:
            m_mask = MASK_32;
            max = 32;
            break;
        case MASK_NONE:
            m_mask = MASK_NONE;
            max = 0;
            break;
    }
}

void StarMaskLoop::setMaskStage(uint32_t mask) {
    m_mask = mask;
}
/*
uint32_t StarMaskLoop::getMaskStage() {
    return m_mask;
}

void StarMaskLoop::setIterations(unsigned it) {
    m_it = it;
}

unsigned StarMaskLoop::getIterations() {
    return m_it;
}*/

void StarMaskLoop::writeConfig(json &config) {
    config["min"] = min;
    config["max"] = max;
    config["step"] = step;
    config["mask"] = (uint32_t) m_mask;
    config["enable_lcap"] = enable_lCap;
    config["enable_scap"] = enable_sCap;
}

void StarMaskLoop::loadConfig(json &config) {
    min = config["min"];
    max = config["max"];
    step = config["step"];
    verbose = config["verbose"];
//    m_mask = (uint32_t) config["mask"];
//    enable_lCap = config["enable_lcap"];
//    enable_sCap = config["enable_scap"];
}
