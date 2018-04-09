// #################################
// # Author: Timon Heim
// # Email: timon.heim at cern.ch
// # Project: Yarr
// # Description: Loop over core columns for RD53A
// # Date: 03/2018
// ################################

#include "Rd53aCoreColLoop.h"

Rd53aCoreColLoop::Rd53aCoreColLoop() : LoopActionBase() {
    min = 0;
    max = 20;
    step = 1;
    m_cur = 0;
    loopType = typeid(this);
    m_done = false;
    verbose = false;
}

void Rd53aCoreColLoop::init() {
    if (verbose)
        std::cout << __PRETTY_FUNCTION__ << std::endl;
    m_done = false;
    m_cur = 0;
}

void Rd53aCoreColLoop::execPart1() {
    if (verbose)
        std::cout << __PRETTY_FUNCTION__ << std::endl;
        
    // Loop over FrontEnds
    for(FrontEnd *fe : keeper->feList) {
        // Loop over cores, i.e. activate in pairs of 4 DC
        for (unsigned dc=0; dc<Rd53a::n_DC; dc+=4) {
            dynamic_cast<Rd53a*>(fe)->disableCalCol(dc);
            dynamic_cast<Rd53a*>(fe)->disableCalCol(dc+1);
            dynamic_cast<Rd53a*>(fe)->disableCalCol(dc+2);
            dynamic_cast<Rd53a*>(fe)->disableCalCol(dc+3);
            if (((dc/4)%max) == m_cur) {
                if (verbose) 
                    std::cout << __PRETTY_FUNCTION__ << " : Enabling QC -> " << dc << std::endl;
                dynamic_cast<Rd53a*>(fe)->enableCalCol(dc);
                dynamic_cast<Rd53a*>(fe)->enableCalCol(dc+1);
                dynamic_cast<Rd53a*>(fe)->enableCalCol(dc+2);
                dynamic_cast<Rd53a*>(fe)->enableCalCol(dc+3);
            }
        }
    }
}

void Rd53aCoreColLoop::execPart2() {
    if (verbose)
        std::cout << __PRETTY_FUNCTION__ << std::endl;
    m_cur += step;
    if (!(m_cur < max)) m_done = true;
    // Nothing else to do here?
    
}

void Rd53aCoreColLoop::end() {
    if (verbose)
        std::cout << __PRETTY_FUNCTION__ << std::endl;

}

void Rd53aCoreColLoop::writeConfig(json &j) {
    j["min"] = min;
    j["max"] = max;
    j["step"] = step;
}

void Rd53aCoreColLoop::loadConfig(json &j) {
    if (!j["min"].empty())
        min = j["min"];
    if (!j["max"].empty())
        max = j["max"];
    if (!j["step"].empty())
        step = j["step"];
}
