// #################################
// # Author: Timon Heim
// # Email: timon.heim at cern.ch
// # Project: Yarr
// # Description: Mask Loop for RD53A
// # Date: 03/2018
// ################################

#include "Rd53aMaskLoop.h"

Rd53aMaskLoop::Rd53aMaskLoop() : LoopActionBase() {
    min = 0;
    max = 32;
    step = 1;
    m_cur = 0;
    loopType = typeid(this);
    m_done = false;
}

void Rd53aMaskLoop::init() {
    if (verbose)
        std::cout << __PRETTY_FUNCTION__ << std::endl;
    m_done = false;
}

void Rd53aMaskLoop::execPart1() {
    if (verbose)
        std::cout << __PRETTY_FUNCTION__ << std::endl;
    
    // Loop over FrontEnds
    for(FrontEnd *fe : keeper->feList) {
        for(unsigned col=0; col<Rd53a::n_Col; col++) {
            for(unsigned row=0; row<Rd53a::n_Row; row++) {
                // Loop in terms of digital cores
                //unsigned core_col = col/8;
                unsigned core_row = row/8;
                // Serialise core column
                unsigned serial = (core_row*64)+(col%8)*8+row%8;
                if (serial%max == m_cur) {
                    dynamic_cast<Rd53a*>(fe)->setEn(col, row, 1);
                    dynamic_cast<Rd53a*>(fe)->setInjEn(col, row, 1);
                } else {
                    dynamic_cast<Rd53a*>(fe)->setEn(col, row, 0);
                    dynamic_cast<Rd53a*>(fe)->setInjEn(col, row, 0);
                }
            }
        }
        // TODO make configrue for subset
        dynamic_cast<Rd53a*>(fe)->configurePixels();
        while(!g_tx->isCmdEmpty()) {}
    }
    g_stat->set(this, m_cur);
    std::cout << " ---> Mask Stage " << m_cur << std::endl;
}

void Rd53aMaskLoop::execPart2() {
    if (verbose)
        std::cout << __PRETTY_FUNCTION__ << std::endl;
    m_cur += step;
    if (!(m_cur < max)) m_done = true;
    // Nothing else to do here?
}

void Rd53aMaskLoop::end() {
    if (verbose)
        std::cout << __PRETTY_FUNCTION__ << std::endl;
    // Nothing to do here?
}

void Rd53aMaskLoop::writeConfig(json &j) {
    j["min"] = min;
    j["max"] = max;
    j["step"] = step;
}

void Rd53aMaskLoop::loadConfig(json &j) {
    if (!j["min"].empty())
        min = j["min"];
    if (!j["max"].empty())
        max = j["step"];
    if (!j["step"].empty())
        step = j["step"];
}
