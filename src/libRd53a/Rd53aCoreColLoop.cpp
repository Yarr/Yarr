// #################################
// # Author: Timon Heim
// # Email: timon.heim at cern.ch
// # Project: Yarr
// # Description: Loop over core columns for RD53A
// # Date: 03/2018
// ################################

#include "Rd53aCoreColLoop.h"
#include "FrontEnd.h"
#include "Rd53a.h"

class Rd53aCoreColLoop::Impl {
    public:
    unsigned m_cur;
    unsigned nSteps;
    unsigned maxCore;
    unsigned minCore;
};


Rd53aCoreColLoop::Rd53aCoreColLoop() : LoopActionBase(), m_impl( new Rd53aCoreColLoop::Impl() ) {
    min = 0;
    max = 50;
    m_impl->nSteps = 25;
    step = 1;
    m_impl->m_cur = 0;
    loopType = typeid(this);
    m_done = false;
    verbose = false;
}

void Rd53aCoreColLoop::init() {
    if (verbose)
        std::cout << __PRETTY_FUNCTION__ << std::endl;
    m_done = false;
    m_impl->m_cur = 0;
    // Disable all to begin with
    for(FrontEnd *fe : keeper->feList) {
        // Loop over cores, i.e. activate in pairs of 4 DC
        for (unsigned dc=0; dc<Rd53a::n_DC; dc+=4) {
            dynamic_cast<Rd53a*>(fe)->disableCalCol(dc);
            dynamic_cast<Rd53a*>(fe)->disableCalCol(dc+1);
            dynamic_cast<Rd53a*>(fe)->disableCalCol(dc+2);
            dynamic_cast<Rd53a*>(fe)->disableCalCol(dc+3);
        }
    }
    while(!g_tx->isCmdEmpty()) {}
}

void Rd53aCoreColLoop::execPart1() {
    if (verbose)
        std::cout << __PRETTY_FUNCTION__ << std::endl;
    
    // Loop over FrontEnds
    for(FrontEnd *fe : keeper->feList) {
        // Loop over cores, i.e. activate in pairs of 4 DC
        for (unsigned dc=(m_impl->minCore*4), i=0; dc<(m_impl->maxCore*4); dc+=4, i++) {
            // Disable previous columns
            if (m_impl->m_cur>0 && ((i%m_impl->nSteps) == (m_impl->m_cur-step))) {
                dynamic_cast<Rd53a*>(fe)->disableCalCol(dc);
                dynamic_cast<Rd53a*>(fe)->disableCalCol(dc+1);
                dynamic_cast<Rd53a*>(fe)->disableCalCol(dc+2);
                dynamic_cast<Rd53a*>(fe)->disableCalCol(dc+3);
            }
            // Enable next columns
            if (i%m_impl->nSteps == m_impl->m_cur) {
                if (verbose)
                    std::cout << __PRETTY_FUNCTION__ << " : Enabling QC -> " << dc << std::endl;
                dynamic_cast<Rd53a*>(fe)->enableCalCol(dc);
                dynamic_cast<Rd53a*>(fe)->enableCalCol(dc+1);
                dynamic_cast<Rd53a*>(fe)->enableCalCol(dc+2);
                dynamic_cast<Rd53a*>(fe)->enableCalCol(dc+3);
            }
        }
    }
    while(!g_tx->isCmdEmpty()) {}
    g_stat->set(this, m_impl->m_cur);
    //std::this_thread::sleep_for(std::chrono::milliseconds(20));
}

void Rd53aCoreColLoop::execPart2() {
    if (verbose)
        std::cout << __PRETTY_FUNCTION__ << std::endl;
    m_impl->m_cur += step;
    if (!(m_impl->m_cur < m_impl->nSteps)) m_done = true;
    // Nothing else to do here?
    
}

void Rd53aCoreColLoop::end() {
    if (verbose)
        std::cout << __PRETTY_FUNCTION__ << std::endl;
    
    /*
    for(FrontEnd *fe : keeper->feList) {
        // Loop over cores, i.e. activate in pairs of 4 DC
        for (unsigned dc=0; dc<Rd53a::n_DC; dc+=4) {
            dynamic_cast<Rd53a*>(fe)->enableCalCol(dc);
            dynamic_cast<Rd53a*>(fe)->enableCalCol(dc+1);
            dynamic_cast<Rd53a*>(fe)->enableCalCol(dc+2);
            dynamic_cast<Rd53a*>(fe)->enableCalCol(dc+3);
        }
    }
    while(!g_tx->isCmdEmpty()) {}
    */
    //std::this_thread::sleep_for(std::chrono::milliseconds(20));
}

void Rd53aCoreColLoop::writeConfig(json &j) {
    j["min"] = m_impl->minCore;
    j["max"] = m_impl->maxCore;
    j["step"] = step;
    j["nSteps"] = m_impl->nSteps;
}

void Rd53aCoreColLoop::loadConfig(json &j) {
    if (!j["min"].empty())
        m_impl->minCore = j["min"];
    if (!j["max"].empty())
        m_impl->maxCore = j["max"];
    if (!j["step"].empty())
        step = j["step"];
    if (!j["nSteps"].empty())
        m_impl->nSteps = j["nSteps"];
    min = 0;
    max = m_impl->nSteps;
}
