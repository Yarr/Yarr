// #################################
// # Author: Timon Heim
// # Email: timon.heim at cern.ch
// # Project: Yarr
// # Description: RD53A Global Feedback Loopaction
// # Comment: 
// # Date: April 2018
// ################################

#include "Rd53aGlobalFeedback.h"

Rd53aGlobalFeedback::Rd53aGlobalFeedback() {
    min = 0;
    max = 255;
    step = 1;
    m_cur = 0;
    loopType = typeid(this);
    m_done = false;
    verbose = false;
}

Rd53aGlobalFeedback::Rd53aGlobalFeedback(Rd53aReg Rd53aGlobalCfg::*ref) : parPtr(ref) {
    min = 0;
    max = 255;
    step = 1;
    m_cur = 0;
    loopType = typeid(this);
    m_done = false;
    verbose = false;
    
}

void Rd53aGlobalFeedback::writeConfig(json &j) {
    j["min"] = min;
    j["max"] = max;
    j["step"] = step;
    j["parameter"] = parName;
}

void Rd53aGlobalFeedback::loadConfig(json &j) {
    if (!j["min"].empty())
        min = j["min"];
    if (!j["max"].empty())
        max = j["max"];
    if (!j["step"].empty())
        step = j["step"];
    if (!j["parameter"].empty()) {
        std::cout << "  Linking parameter: " << j["parameter"] <<std::endl;
        parName = j["parameter"];
    }

}

void Rd53aGlobalFeedback::feedback(unsigned channel, double sign, bool last) {
    // Calculate new step and val
    if (sign != m_oldSign[channel]) {
        m_oldSign[channel] = 0;
        m_localStep[channel] = m_localStep[channel]/2;
    }
    int val = (m_values[channel]+(m_localStep[channel]*sign));
    if (val > (int)max) val = max;
    if (val < 0) val = 0;
    m_values[channel] = val;
    m_doneMap[channel] |= last;

    if (m_localStep[channel] == 1) {
        doneMap[channel] = true;
    }

    // Abort if we are getting to low
    if (val < min) {
        doneMap[channel] = true;
    }
    // Unlock the mutex to let the scan proceed
    keeper->mutexMap[channel].unlock();
}

void Rd53aGlobalFeedback::feedbackBinary(unsigned channel, double sign, bool last) {
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
    keeper->mutexMap[channel].unlock();

}

bool Rd53aGlobalFeedback::allDone() {
    for (auto *fe : keeper->feList) {
        if (fe->getActive()) {
            if (!m_doneMap[dynamic_cast<FrontEndCfg*>(fe)->getRxChannel()])
                return false;
        }
    }
    return true;
}

void Rd53aGlobalFeedback::writePar() {
    for (auto *fe : keeper->feList) {
        if(fe->getActive()) {
            // Enable single channel
            g_tx->setCmdEnable(1 << dynamic_cast<FrontEndCfg*>(fe)->getTxChannel());
            // Write parameter
            dynamic_cast<Rd53a*>(fe)->writeRegister(parPtr, m_values[dynamic_cast<FrontEndCfg*>(fe)->getRxChannel()]);
            while(!g_tx->isCmdEmpty()){}
        }
    }
    // Reset CMD mask
    g_tx->setCmdEnable(keeper->getTxMask());
}

void Rd53aGlobalFeedback::init() {
    if (verbose)
        std::cout << __PRETTY_FUNCTION__ << std::endl;
    m_done = false;
    m_cur = min;
    parPtr = keeper->globalFe<Rd53a>()->regMap[parName];
    // Init maps
    for (auto *fe : keeper->feList) {
        if (fe->getActive()) {
            unsigned ch = dynamic_cast<FrontEndCfg*>(fe)->getRxChannel();
            m_localStep[ch] = step;
            m_values[ch] = max;
            m_oldSign[ch] = -1;
            m_doneMap[ch] = false;
        }
    }
    this->writePar();
}

void Rd53aGlobalFeedback::execPart1() {
    g_stat->set(this, m_cur);
    // Lock all mutexes
    for (auto fe : keeper->feList) {
        if (fe->getActive()) {
            keeper->mutexMap[dynamic_cast<FrontEndCfg*>(fe)->getRxChannel()].try_lock();
        }
    }
    m_done = this->allDone();
}

void Rd53aGlobalFeedback::execPart2() {
    // Wait for mutexes to be unlocked by feedback
    for (auto fe: keeper->feList) {
        if (fe->getActive()) {
            unsigned rx = dynamic_cast<FrontEndCfg*>(fe)->getRxChannel();
            keeper->mutexMap[rx].lock();
            std::cout << " --> Received Feedback on Channel " << rx << " with value: " << m_values[rx] << std::endl;
        }
    }
    m_cur++;
    this->writePar();
}

void Rd53aGlobalFeedback::end() {
    for (auto fe: keeper->feList) {
        if (fe->getActive()) {
            unsigned rx = dynamic_cast<FrontEndCfg*>(fe)->getRxChannel();
            keeper->mutexMap[rx].lock();
            std::cout << " --> Final parameter for Channel " << rx << " is " << m_values[rx] << std::endl;
        }
    }
    this->writePar();
}
