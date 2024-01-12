// #################################
// # Author: Timon Heim
// # Email: timon.heim at cern.ch
// # Project: Yarr
// # Description: RD53A Global Feedback Loopaction
// # Comment: 
// # Date: April 2018
// ################################

#include "Rd53aGlobalFeedback.h"

#include "logging.h"

namespace {
  auto logger = logging::make_log("Rd53aGlobalFeedback");
}

Rd53aGlobalFeedback::Rd53aGlobalFeedback() : LoopActionBase(LOOP_STYLE_GLOBAL_FEEDBACK) {
    min = 0;
    max = 255;
    step = 1;
    m_cur = 0;
    loopType = typeid(this);
    m_done = false;
    m_pixelReg.resize(2);
    m_pixelReg = {0, 0};
}

Rd53aGlobalFeedback::Rd53aGlobalFeedback(Rd53Reg Rd53aGlobalCfg::*ref) : LoopActionBase(LOOP_STYLE_GLOBAL_FEEDBACK), parPtr(ref) {
    min = 0;
    max = 255;
    step = 1;
    m_cur = 0;
    loopType = typeid(this);
    m_done = false;
    m_pixelReg.resize(2);
    m_pixelReg = {0, 0};
}

void Rd53aGlobalFeedback::writeConfig(json &j) {
    j["min"] = min;
    j["max"] = max;
    j["step"] = step;
    j["parameter"] = parName;
    j["pixelRegs"] = m_pixelReg;
}

void Rd53aGlobalFeedback::loadConfig(const json &j) {
    if (j.contains("min"))
        min = j["min"];
    if (j.contains("max"))
        max = j["max"];
    if (j.contains("step"))
        step = j["step"];
    if (j.contains("parameter")) {
        logger->info("Linking parameter: {}", std::string(j["parameter"]));
        parName = j["parameter"];
    }
    if (j.contains("pixelRegs")) {
        m_pixelReg.clear();
        for(auto i: j["pixelRegs"])
            m_pixelReg.push_back(i);
        if (m_pixelReg.size() != 2) {
            logger->error("Expected 2 values, got {}", m_pixelReg.size());
        }
    }
}

void Rd53aGlobalFeedback::feedback(unsigned channel, double sign, bool last) {
    // Calculate new step and val
    logger->debug("[{}] Received feedback {} (old: {})", channel, sign, m_oldSign[channel]);    
    if (sign != m_oldSign[channel]) {
        m_oldSign[channel] = 0;
        m_localStep[channel] = m_localStep[channel]/2;
    }
    int val = (m_values[channel]+(m_localStep[channel]*sign));
    if (val > (int)max) val = max;
    if (val < min) val = min;
    m_values[channel] = val;
    fbDoneMap[channel] |= last;

    if (m_localStep[channel] == 1 || val == min) {
        fbDoneMap[channel] = true;
    }

    // Abort if we are getting to low
    if (val <= min) {
        fbDoneMap[channel] = true;
    }
}

void Rd53aGlobalFeedback::feedbackBinary(unsigned channel, double sign, bool last) {
    // Calculate new step and value
    int val = (m_values[channel]+(m_localStep[channel]*sign));
    if (val < 0) val = 0;
    m_values[channel] = val;
    m_localStep[channel]  = m_localStep[channel]/2;
    fbDoneMap[channel] |= last;

    if (m_localStep[channel] == 1) {
        fbDoneMap[channel] = true;
    }
}

void Rd53aGlobalFeedback::feedbackStep(unsigned channel, double sign, bool last) {
    m_values[channel] = m_values[channel] + sign;
    fbDoneMap[channel] |= last;
}


void Rd53aGlobalFeedback::writePar() {
    for (unsigned id=0; id<keeper->getNumOfEntries(); id++) {
        auto fe = keeper->getFe(id);
        if(fe->getActive()) {
            auto feCfg = dynamic_cast<FrontEndCfg*>(fe);
            // Enable single channel
            g_tx->setCmdEnable(feCfg->getTxChannel());
            // Write parameter
            dynamic_cast<Rd53a*>(fe)->writeRegister(parPtr, m_values[id]);
            while(!g_tx->isCmdEmpty()){}
        }
    }
    // Reset CMD mask
    g_tx->setCmdEnable(keeper->getTxMask());
}

void Rd53aGlobalFeedback::init() {
    logger->debug("init");
    m_done = false;
    m_cur = 0;
    parPtr = keeper->globalFe<Rd53a>()->regMap[parName];
    // Init maps
    for (unsigned id=0; id<keeper->getNumOfEntries(); id++) {
        auto fe = keeper->getFe(id);
        if (fe->getActive()) {
            m_localStep[id] = step;
            m_values[id] = max;
            m_oldSign[id] = -1;
            fbDoneMap[id] = false;
        }
    }
    this->writePar();

    for (unsigned id=0; id<keeper->getNumOfEntries(); id++) {
        auto fe = keeper->getFe(id);
        if (fe->getActive()) {
            Rd53a *rd53a = dynamic_cast<Rd53a*>(fe);
            g_tx->setCmdEnable(dynamic_cast<FrontEndCfg*>(fe)->getTxChannel());
            switch (m_pixelReg[0]) { // Lin regs
                case 0: // Leave TDACs as config setting
                    break;
                case 1: // Put TDACs to default
                    for (unsigned col=129; col<=264; col++) {
                        for (unsigned row=1; row<=Rd53a::n_Row; row++) {
                            rd53a->setTDAC(col-1, row-1, 8);
                        }
                    }
                    break;
                case 2: // Put TDACs to max
                    for (unsigned col=129; col<=264; col++) {
                        for (unsigned row=1; row<=Rd53a::n_Row; row++) {
                            rd53a->setTDAC(col-1, row-1, 0);
                        }
                    }
                    break;
                case 3: // Put TDACs to min
                    for (unsigned col=129; col<=264; col++) {
                        for (unsigned row=1; row<=Rd53a::n_Row; row++) {
                            rd53a->setTDAC(col-1, row-1, 15);
                        }
                    }
                    break;
                default:
                    break;
            }
            switch (m_pixelReg[1]) { // Diff regs
                case 0: // Leave TDACs as config setting
                    break;
                case 1: // Put TDACs to default
                    for (unsigned col=265; col<=400; col++) {
                        for (unsigned row=1; row<=Rd53a::n_Row; row++) {
                            rd53a->setTDAC(col-1, row-1, 0);
                        }
                    }
                    break;
                case 2: // Put TDACs to max
                    for (unsigned col=265; col<=400; col++) {
                        for (unsigned row=1; row<=Rd53a::n_Row; row++) {
                            rd53a->setTDAC(col-1, row-1, 15);
                        }
                    }
                    break;
                case 3: // Put TDACs to min
                    for (unsigned col=265; col<=400; col++) {
                        for (unsigned row=1; row<=Rd53a::n_Row; row++) {
                            rd53a->setTDAC(col-1, row-1, -15);
                        }
                    }
                    break;
                default:
                    break;
            }
        }
        while(!g_tx->isCmdEmpty()){}
    }
    g_tx->setCmdEnable(keeper->getTxMask());
}

void Rd53aGlobalFeedback::execPart1() {
    g_stat->set(this, m_cur);
}

void Rd53aGlobalFeedback::execPart2() {
    // Wait for mutexes to be unlocked by feedback
    for (unsigned id=0; id<keeper->getNumOfEntries(); id++) {
        auto fe = keeper->getFe(id);
        if (fe->getActive()) {
            waitForFeedback(id);
            logger->info(" --> Received Feedback on ID {} with value: {}", id, m_values[id]);
        }
    }
    m_cur++;
    this->writePar();
    m_done = this->isFeedbackDone();
}

void Rd53aGlobalFeedback::end() {
    for (unsigned id=0; id<keeper->getNumOfEntries(); id++) {
        auto fe = keeper->getFe(id);
        if (fe->getActive()) {
            logger->info(" --> Final parameter for Id {} is {}", id, m_values[id]);
        }
    }
    this->writePar();
}
