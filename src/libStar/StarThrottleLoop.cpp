// #################################
// # Author:
// # Email:
// # Project: Yarr
// # Description: Star Global Feedback Loop action
// # Comment: 
// # Date: Oct 2018
// ################################

#include "include/StarGlobalFeedback.h"

StarGlobalFeedback::StarGlobalFeedback() : LoopActionBase(LOOP_STYLE_GLOBAL_FEEDBACK) {
    min = 0;
    max = 255;
    step = 1;
    m_cur = 0;
    loopType = typeid(this);
    m_done = false;
}

StarGlobalFeedback::StarGlobalFeedback(Register StarCfg::*ref) : LoopActionBase(LOOP_STYLE_GLOBAL_FEEDBACK), parPtr(ref) {
    min = 0;
    max = 255;
    step = 1;
    m_cur = 0;
    loopType = typeid(this);
    m_done = false;
}

void StarGlobalFeedback::writeConfig(json &j) {
    j["min"] = min;
    j["max"] = max;
    j["step"] = step;
    j["parameter"] = m_subRegName;
}

void StarGlobalFeedback::loadConfig(json &j) {
    if (!j["min"].empty())
        min = j["min"];
    if (!j["max"].empty())
        max = j["max"];
    if (!j["step"].empty())
        step = j["step"];
    if (!j["parameter"].empty()) {
        std::cout << "  Linking parameter: " << j["parameter"] <<std::endl;
        m_subRegName = j["parameter"];
    }

}

void StarGlobalFeedback::feedback(unsigned channel, bool stop) {

//    std::cout << __PRETTY_FUNCTION__ <<  std::endl;
	// Calculate new step and val

	int val = m_values[channel];
    if (stop){
    	m_values[channel]-=m_localStep[channel];
    	std::cout << "Changing " <<  m_subRegName <<  "\033[1;35m" << " from " << val << "  to " << m_values[channel]  << "\033[0m" <<std::endl;
    }


    // Abort if we are getting to low
    if (m_values[channel] < min) {
        m_doneMap[channel] = true;
    }
    // Unlock the mutex to let the scan proceed
    //keeper->mutexMap[channel].unlock();
}

void StarGlobalFeedback::feedback(unsigned channel, double sign, bool last) {
    // Calculate new step and val
//    std::cout << __PRETTY_FUNCTION__ << " : " << channel << " " << sign << " " << m_oldSign[channel] << " done: " << last << std::endl;
    if (sign != m_oldSign[channel]) {
        m_oldSign[channel] = 0;
        m_localStep[channel] = m_localStep[channel]/2;
    }
    int val = (m_values[channel]+(m_localStep[channel]*sign));
    if (val > (int)max) val = max;
    if (val < min) val = min;
    std::cout << "Changing " <<  m_subRegName <<  "\033[1;35m" << " from " << m_values[channel] << "  to " << val << "\033[0m" <<std::endl;
    m_values[channel] = val;
    m_doneMap[channel] |= last;


    if (m_localStep[channel] == 1 || val == min) {
        m_doneMap[channel] = true;
    }

    // Abort if we are getting to low
    if (val <= min) {
        m_doneMap[channel] = true;
    }
    // Unlock the mutex to let the scan proceed
    //keeper->mutexMap[channel].unlock();
}

void StarGlobalFeedback::feedbackBinary(unsigned channel, double sign, bool last) {
    // Calculate new step and value
    int val = (m_values[channel]+(m_localStep[channel]*sign));
    if (val < 0) val = 0;
    m_values[channel] = val;
    m_localStep[channel]  = m_localStep[channel]/2;
    m_doneMap[channel] |= last;

    if (m_localStep[channel] == 1) {
        m_doneMap[channel] = true;
    }

    // Unlock the mutex to let the scan proceed
    //keeper->mutexMap[channel].unlock();
}

bool StarGlobalFeedback::allDone() {
    for (auto *fe : keeper->feList) {
//    	if (dynamic_cast<FrontEndCfg*>(fe)->getName().find("hcc") != std::string::npos) {continue;}  ///fix me, maybe @@@
        if (fe->getActive()) {
            if (!m_doneMap[dynamic_cast<FrontEndCfg*>(fe)->getRxChannel()])
                return false;
        }
    }
    return true;
}

void StarGlobalFeedback::writePar() {

	for ( auto* fe : keeper->feList ) {
//		if (dynamic_cast<FrontEndCfg*>(fe)->getName().find("hcc") != std::string::npos) {continue;}  ///fix me, maybe @@@
		if (!fe->isActive()) {continue;}
                static_cast<StarChips*> (fe)->writeNamedRegister(m_subRegName, m_values[dynamic_cast<FrontEndCfg*>(fe)->getRxChannel()]);
        }
	// 	for( int iChip = 1; iChip < static_cast<StarChips*> (fe)->m_nABC+1; ++iChip){ //exclude iChip=0 which is the Hcc
	// 		//static_cast<Star*> (fe)->setRunMode();
	// 		int this_chipID = static_cast<StarChips*> (fe)->getABCchipID(iChip);
	// 		//			std::this_thread::sleep_for(std::chrono::milliseconds(1000));
	// 		static_cast<StarChips*> (fe)->setAndWriteABCSubRegister(m_subRegName, m_values[dynamic_cast<FrontEndCfg*>(fe)->getRxChannel()], this_chipID);
	// 		//		std::this_thread::sleep_for(std::chrono::milliseconds(10));
	// 		//		while(!g_tx->isCmdEmpty()){}
	// 	}
	// }


//    for (auto *fe : keeper->feList) {
//        if(fe->getActive()) {
//            // Enable single channel
//            g_tx->setCmdEnable(1 << dynamic_cast<FrontEndCfg*>(fe)->getTxChannel());
//            // Write parameter
//            dynamic_cast<Star*>(fe)->writeRegister(parPtr, m_values[dynamic_cast<FrontEndCfg*>(fe)->getRxChannel()]);
//            while(!g_tx->isCmdEmpty()){}
//        }
//    }
//    // Reset CMD mask
//    g_tx->setCmdEnable(keeper->getTxMask());
}

void StarGlobalFeedback::init() {
    std::cout << __PRETTY_FUNCTION__ << std::endl;
    m_done = false;

//    subRegPtr = keeper->globalFe<Star>()->subRegisterMap_all[m_subRegName];
    // Init maps
    for (auto *fe : keeper->feList) {
//    	if (dynamic_cast<FrontEndCfg*>(fe)->getName().find("hcc") != std::string::npos) {continue;}  ///fix me, maybe @@@
        if (fe->getActive()) {

            unsigned ch = dynamic_cast<FrontEndCfg*>(fe)->getRxChannel();
//            std::cout << __PRETTY_FUNCTION__ << "  rxchannel: "<< ch  << std::endl;
            m_localStep[ch] = step;
            m_values[ch] = max;
            m_oldSign[ch] = -1;
            m_doneMap[ch] = false;
            m_cur = m_values[ch] ;
        }
    }

    this->writePar();
}

void StarGlobalFeedback::execPart1() {
//	if (verbose)
//		std::cout << __PRETTY_FUNCTION__ << std::endl;

	g_stat->set(this, m_cur);
        /*
    // Lock all mutexes
    for (auto fe : keeper->feList) {
//    	if (dynamic_cast<FrontEndCfg*>(fe)->getName().find("hcc") != std::string::npos) {continue;}  ///fix me, maybe @@@
        if (fe->getActive()) {
            keeper->mutexMap[dynamic_cast<FrontEndCfg*>(fe)->getRxChannel()].try_lock();
        }
    }
    m_done = this->allDone();*/
}

void StarGlobalFeedback::execPart2() {
//	std::cout << __PRETTY_FUNCTION__ << std::endl;
    // Wait for mutexes to be unlocked by feedback
    for (auto fe: keeper->feList) {
//    	if (dynamic_cast<FrontEndCfg*>(fe)->getName().find("hcc") != std::string::npos) {continue;}  ///fix me, maybe @@@
        if (fe->getActive()) {
            unsigned rx = dynamic_cast<FrontEndCfg*>(fe)->getRxChannel();
//            std::cout << " --> Received Feedback on Channel " << rx << " with value: " << ml_values[rx] << std::endl;
            waitForFeedback(rx);
            m_cur = m_values[rx];
        }
    }

    this->writePar();
    m_done = this->allDone();
}

void StarGlobalFeedback::end() {
//	std::cout << __PRETTY_FUNCTION__ << std::endl;
    for (auto fe: keeper->feList) {
        //    	if (dynamic_cast<FrontEndCfg*>(fe)->getName().find("hcc") != std::string::npos) {continue;}  ///fix me, maybe @@@
    	if (fe->getActive()) {
            unsigned rx = dynamic_cast<FrontEndCfg*>(fe)->getRxChannel();
            //    		for( int iChip = 1; iChip < static_cast<StarChips*> (fe)->m_nABC+1; ++iChip){ //exclude iChip=0 which is the Hcc
            //	int this_chipID = static_cast<StarChips*> (fe)->getABCchipID(iChip);
            //    			unsigned chipId = dynamic_cast<Star*>(fe)->getChipId();
            //	std::cout << "\033[1;33m" << " --> Final " << m_subRegName << " value for Abc " << this_chipID << " in e-link "<< rx<< " is " << m_values[rx] << "\033[0m" <<  std::endl;
        }
    }
    this->writePar();
}
