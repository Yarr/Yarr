// #################################
// # Author: Timon Heim
// # Email: timon.heim at cern.ch
// # Project: Yarr
// # Description: RD53A Global Feedback Loopaction
// # Comment: 
// # Date: April 2018
// ################################

#include "Rd53aPixelFeedback.h"

Rd53aPixelFeedback::Rd53aPixelFeedback() {
    min = 0;
    max = 15;
    step = 1;
    m_cur = 0;
    loopType = typeid(this);
    m_done = false;
    verbose = false;
    tuneLin = true;
    tuneDiff = true;
}

void Rd53aPixelFeedback::writeConfig(json &j) {
    j["min"] = min;
    j["max"] = max;
    j["step"] = step;
    j["tuneDiff"] = tuneDiff;
    j["tuneLin"] = tuneLin;
}

void Rd53aPixelFeedback::loadConfig(json &j) {
    if (!j["min"].empty())
        min = j["min"];
    if (!j["max"].empty())
        max = j["max"];
    if (!j["step"].empty())
        step = j["step"];
    if (!j["tuneDiff"].empty())
        tuneDiff = j["tuneDiff"];
    if (!j["tuneLin"].empty())
        tuneLin = j["tuneLin"];
}

void Rd53aPixelFeedback::feedback(unsigned channel, Histo2d *h) {
    // TODO Check on NULL pointer
    if (h->size() != Rd53a::n_Row*Rd53a::n_Col) {
        std::cout << __PRETTY_FUNCTION__ 
            << " --> ERROR : Wrong type of feedback histogram on channel " << channel << std::endl;
        doneMap[channel] = true;
    } else {
        m_fb[channel] = h;
    }
    keeper->mutexMap[channel].unlock();
}

void Rd53aPixelFeedback::addFeedback(unsigned ch) {
    if (m_fb[ch] != NULL) {
        int linCnt = 0;
        int diffCnt = 0;
        for (unsigned row=1; row<=Rd53a::n_Row; row++) {
            for (unsigned col=1; col<=Rd53a::n_Col; col++) {
                int sign = m_fb[ch]->getBin(m_fb[ch]->binNum(col, row));
                int v = dynamic_cast<Rd53a*>(keeper->getFe(ch))->getTDAC(col-1, row-1);
                if (128<col && col<=264 && tuneLin) {
                    linCnt++;
                    v = v + (step*sign);
                } else if (264<col && tuneDiff) {
                    diffCnt++;
                    v = v + (step*sign*-1);
                }
                if (v<min) v = min;
                if (v>max) v = max;
                dynamic_cast<Rd53a*>(keeper->getFe(ch))->setTDAC(col-1, row-1, v);
            }
        }
        std::cout << "Modified " << linCnt << " lin FE pixels!" << std::endl;
        std::cout << "Modified " << diffCnt << " diff FE pixels!" << std::endl;
    }
}

void Rd53aPixelFeedback::writePixelCfg(Rd53a *fe) {
    g_tx->setCmdEnable(1 << dynamic_cast<FrontEndCfg*>(fe)->getTxChannel());
    fe->configurePixels();
    g_tx->setCmdEnable(keeper->getTxMask());
    while(!g_tx->isCmdEmpty());
}

void Rd53aPixelFeedback::init() {
    if (1)
        std::cout << __PRETTY_FUNCTION__ << std::endl;
    m_done = false;
    m_cur = 0;
    oldStep = step;
    // Init maps
    for (auto *fe : keeper->feList) {
        if (fe->getActive()) {
            unsigned ch = dynamic_cast<FrontEndCfg*>(fe)->getRxChannel();
            m_fb[ch] = NULL;
            int linCnt = 0;
            int diffCnt = 0;
            for (unsigned col=1; col<=Rd53a::n_Col; col++) {
                for (unsigned row=1; row<=Rd53a::n_Row; row++) {
                    //Initial TDAC in mid of the range
                    if (128<col && col<=264 && tuneLin) {
                        dynamic_cast<Rd53a*>(keeper->getFe(ch))->setTDAC(col-1, row-1, 7);
                        linCnt++;
                    } else if (264<col && tuneDiff) {
                        dynamic_cast<Rd53a*>(keeper->getFe(ch))->setTDAC(col-1, row-1, 0);
                        diffCnt++;
                    }
                }
            }
            std::cout << "Reset " << linCnt << " lin FE pixels!" << std::endl;
            std::cout << "Reset " << diffCnt << " diff FE pixels!" << std::endl;
        }
                
    }
}

void Rd53aPixelFeedback::execPart1() {
    g_stat->set(this, m_cur);
    // Lock all mutexes
    for (auto fe : keeper->feList) {
        if (fe->getActive()) {
            keeper->mutexMap[dynamic_cast<FrontEndCfg*>(fe)->getRxChannel()].try_lock();
            this->writePixelCfg(dynamic_cast<Rd53a*>(fe));
        }
    }
}

void Rd53aPixelFeedback::execPart2() {
    // Wait for mutexes to be unlocked by feedback
    for (auto fe: keeper->feList) {
        if (fe->getActive()) {
            unsigned rx = dynamic_cast<FrontEndCfg*>(fe)->getRxChannel();
            keeper->mutexMap[rx].lock();
            this->addFeedback(rx);
        }
    }
    // Execute last step twice to get full range
    if (step == 1 && oldStep == 1)
        m_done = true;
    oldStep = step;
    step = step/2;
    if(step == 0)
        step = 1;

    m_cur++;
}

void Rd53aPixelFeedback::end() {
    /*
    for (auto fe: keeper->feList) {
        if (fe->getActive()) {
            unsigned rx = dynamic_cast<FrontEndCfg*>(fe)->getRxChannel();
            keeper->mutexMap[rx].lock();
            this->writePixelCfg(dynamic_cast<Rd53a*>(fe));
        }
    }
    */
}
