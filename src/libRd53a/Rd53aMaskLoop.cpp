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
    verbose = false;
    goodPixels = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64};
}

void Rd53aMaskLoop::init() {
    if (verbose)
        std::cout << __PRETTY_FUNCTION__ << std::endl;
    m_done = false;
    m_cur = min;
    for(FrontEnd *fe : keeper->feList) {
        // Make copy of pixRegs
        m_pixRegs[fe] = dynamic_cast<Rd53a*>(fe)->pixRegs;
        g_tx->setCmdEnable(1 << dynamic_cast<FrontEndCfg*>(fe)->getTxChannel());
        for(unsigned col=0; col<Rd53a::n_Col; col++) {
            for(unsigned row=0; row<Rd53a::n_Row; row++) {
                dynamic_cast<Rd53a*>(fe)->setEn(col, row, 0);
                dynamic_cast<Rd53a*>(fe)->setInjEn(col, row, 0);
            }
        }
        // TODO make configrue for subset
        dynamic_cast<Rd53a*>(fe)->configurePixels();
        while(!g_tx->isCmdEmpty()) {}
    }
    // Reset CMD mask
    g_tx->setCmdEnable(keeper->getTxMask());
    
}

void Rd53aMaskLoop::execPart1() {
    if (verbose)
        std::cout << __PRETTY_FUNCTION__ << std::endl;
    bool pixelsEnabled = false;
    // Loop over FrontEnds
    while (!pixelsEnabled && ((int)m_cur < max)){
        for(FrontEnd *fe : keeper->feList) {
            g_tx->setCmdEnable(1 << dynamic_cast<FrontEndCfg*>(fe)->getTxChannel());
            std::vector<std::pair<unsigned, unsigned>> modPixels;
            for(unsigned col=0; col<Rd53a::n_Col; col++) {
                for(unsigned row=0; row<Rd53a::n_Row; row++) {
                    // Loop in terms of digital cores
                    //unsigned core_col = col/8;
                    unsigned core_row = row/8;
                    // Serialise core column
                    unsigned serial = (core_row*64)+((col)%8)*8+row%8; //(core_row%8) = 0
                    //unsigned serial = (core_row*64)+(col%8)*8+row%8;
                    //Look for mask stage in goodpixels
                    std::vector<int>::iterator it = std::find(goodPixels.begin(), goodPixels.end(), (core_row*64+(col%8)*8+row%8)%64);
                    if ((serial%64 == m_cur) & (it != goodPixels.end())) {
                        dynamic_cast<Rd53a*>(fe)->setEn(col, row, 1);
                        dynamic_cast<Rd53a*>(fe)->setInjEn(col, row, 1);
                        modPixels.push_back(std::make_pair(col, row));
                        pixelsEnabled = true;
                    } else {
                        if (dynamic_cast<Rd53a*>(fe)->getInjEn(col, row) == 1) {
                            dynamic_cast<Rd53a*>(fe)->setEn(col, row, 0);
                            dynamic_cast<Rd53a*>(fe)->setInjEn(col, row, 0);
                            modPixels.push_back(std::make_pair(col, row));
                        }
                    }
                }
            }
            // TODO make configure for subset
            // TODO set cmeEnable correctly
            dynamic_cast<Rd53a*>(fe)->configurePixels(modPixels);
            while(!g_tx->isCmdEmpty()) {}
        }
        if(!pixelsEnabled){
            std::cout << "For mask stage " << m_cur << " no pixels were enabled" << std::endl;
            m_cur += step;
        }
    }
    // Reset CMD mask
    g_tx->setCmdEnable(keeper->getTxMask());
    std::cout << " ---> Mask Stage " << m_cur << std::endl;
    g_stat->set(this, m_cur);
    //std::this_thread::sleep_for(std::chrono::milliseconds(20));
}

void Rd53aMaskLoop::execPart2() {
    if (verbose)
        std::cout << __PRETTY_FUNCTION__ << std::endl;
    m_cur += step;
    if (!((int)m_cur < max)) m_done = true;
    // Nothing else to do here?
}

void Rd53aMaskLoop::end() {
    if (verbose)
        std::cout << __PRETTY_FUNCTION__ << std::endl;
    
    for(FrontEnd *fe : keeper->feList) {
        // Copy original registers back
        // TODO need to make sure analysis modifies the right config
        // TODO not thread safe
        dynamic_cast<Rd53a*>(fe)->pixRegs = m_pixRegs[fe];
    }
    // Reset CMD mask
    g_tx->setCmdEnable(keeper->getTxMask());

    // Nothing to do here?
}

void Rd53aMaskLoop::writeConfig(json &j) {
    j["min"] = min;
    j["max"] = max;
    j["step"] = step;
}

void Rd53aMaskLoop::loadConfig(json &j) {
    std::cout << "Mask stage loadConfig" << std::endl;
    if (!j["min"].empty()){
        min = j["min"];
        m_cur = min;
    }
    if (!j["max"].empty())
        max = j["max"];
    if (!j["step"].empty())
        step = j["step"];
    if (!j["goodPixels"].empty()){
        try{
            goodPixels = j["goodPixels"].get<std::vector<int>>();
        }catch(...){
            std::cout << "goodPixels error. If it is given as single value, ignore this error." << std::endl;
            goodPixels = {j["goodPixels"]};
        }
    }
}

void Rd53aMaskLoop::calcNruns() {
    //calculate how many mask stages will be executed
    bool pixelsEnabled;
    uint32_t m_cur_tmp = 0;
    uint32_t Nruns_tmp = 0;
    while (((int)m_cur_tmp < max)){
        pixelsEnabled = false;
        for(unsigned i=0; i < keeper->feList.size(); i++) {
            std::vector<std::pair<unsigned, unsigned>> modPixels;
            for(unsigned col=0; col<Rd53a::n_Col; col++) {
                for(unsigned row=0; row<Rd53a::n_Row; row++) {
                    // Loop in terms of digital cores
                    //unsigned core_col = col/8;
                    unsigned core_row = row/8;
                    // Serialise core column
                    unsigned serial = (core_row*64)+((col)%8)*8+row%8; //(core_row%8) = 0
                    //unsigned serial = (core_row*64)+(col%8)*8+row%8;
                    //Look for mask stage in goodpixels
                    std::vector<int>::iterator it = std::find(goodPixels.begin(), goodPixels.end(), (core_row*64+(col%8)*8+row%8)%64);
                    if ((serial%64 == m_cur_tmp) & (it != goodPixels.end())) {
                        pixelsEnabled = true;
                        Nruns_tmp += 1;
                        break;
                    }
                }
                if(pixelsEnabled){break;}
            }
            if(pixelsEnabled){break;}
        }
        m_cur_tmp += step;
    }
    Nruns = Nruns_tmp;
}