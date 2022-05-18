// #################################
// # Author: Timon Heim
// # Email: timon.heim at cern.ch
// # Project: Yarr
// # Description: RD53B Global Feedback Loopaction
// # Date: July 2020
// ################################

#include "Rd53bGlobalFeedback.h"

#include "logging.h"

namespace {
    auto logger = logging::make_log("Rd53bGlobalFeedback");
}

Rd53bGlobalFeedback::Rd53bGlobalFeedback() : LoopActionBase(LOOP_STYLE_GLOBAL_FEEDBACK) {
    min = 0;
    max = 255;
    step = 1;
    m_cur = 0;
    loopType = typeid(this);
    m_done = false;
    m_rstPixelReg = true;
    m_pixelReg = 0;
}

Rd53bGlobalFeedback::Rd53bGlobalFeedback(Rd53bReg Rd53bGlobalCfg::*ref) : LoopActionBase(LOOP_STYLE_GLOBAL_FEEDBACK), parPtr(ref) {
    min = 0;
    max = 255;
    step = 1;
    m_cur = 0;
    loopType = typeid(this);
    m_done = false;
    m_rstPixelReg = true;
    m_pixelReg = 0;
}

void Rd53bGlobalFeedback::writeConfig(json &j) {
    j["min"] = min;
    j["max"] = max;
    j["step"] = step;
    j["parameter"] = parName;
    j["pixelReg"] = m_pixelReg;
    j["rstPixelReg"] = m_rstPixelReg;
}

void Rd53bGlobalFeedback::loadConfig(const json &j) {
    if (j.contains("min"))
        min = j["min"];
    if (j.contains("max"))
        max = j["max"];
    if (j.contains("step"))
        step = j["step"];
    if (j.contains("pixelReg"))
        m_pixelReg = j["pixelReg"];
    if (j.contains("rstPixelReg"))
        m_rstPixelReg = j["rstPixelReg"];
    if (j.contains("parameter")) {
        logger->info("Linking parameter: {}", std::string(j["parameter"]));
        parName = j["parameter"];
    }
}

void Rd53bGlobalFeedback::feedback(unsigned channel, double sign, bool last) {
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
    m_doneMap[channel] |= last;

    if (m_localStep[channel] == 1 || val == min) {
        m_doneMap[channel] = true;
    }

    // Abort if we are getting to low
    if (val <= min) {
        m_doneMap[channel] = true;
    }
}

void Rd53bGlobalFeedback::feedbackBinary(unsigned channel, double sign, bool last) {
    // Calculate new step and value
    int val = (m_values[channel]+(m_localStep[channel]*sign));
    if (val < 0) val = 0;
    m_values[channel] = val;
    m_localStep[channel]  = m_localStep[channel]/2;
    m_doneMap[channel] |= last;

    if (m_localStep[channel] == 1) {
        m_doneMap[channel] = true;
    }
    if (m_values[channel] == 0) {
        m_doneMap[channel] = true;
    }
}

void Rd53bGlobalFeedback::feedbackStep(unsigned channel, double sign, bool last) {
    m_values[channel] = m_values[channel] + sign;
    m_doneMap[channel] |= last;
}


bool Rd53bGlobalFeedback::allDone() {
    for (unsigned id=0; id<keeper->getNumOfEntries(); id++) {
        FrontEnd *fe = keeper->getEntry(id).fe;
        if (fe->getActive()) {
            if (!m_doneMap[dynamic_cast<FrontEndCfg*>(fe)->getRxChannel()])
                return false;
        }
    }
    return true;
}

void Rd53bGlobalFeedback::writePar() {
    for (unsigned id=0; id<keeper->getNumOfEntries(); id++) {
        FrontEnd *fe = keeper->getEntry(id).fe;
        if(fe->getActive()) {
            auto feCfg = dynamic_cast<FrontEndCfg*>(fe);
            // Enable single channel
            g_tx->setCmdEnable(feCfg->getTxChannel());
            // Write parameter
            dynamic_cast<Rd53b*>(fe)->writeRegister(parPtr, m_values[feCfg->getRxChannel()]);
            while(!g_tx->isCmdEmpty()){}
        }
    }
    // Reset CMD mask
    g_tx->setCmdEnable(keeper->getTxMask());
}

void Rd53bGlobalFeedback::init() {
    logger->debug("init");
    m_done = false;
    m_cur = 0;
    parPtr = keeper->globalFe<Rd53b>()->getNamedRegister(parName);
    // Init maps
    for (unsigned id=0; id<keeper->getNumOfEntries(); id++) {
        FrontEnd *fe = keeper->getEntry(id).fe;
        if (fe->getActive()) {
            unsigned ch = dynamic_cast<FrontEndCfg*>(fe)->getRxChannel();
            m_localStep[ch] = step;
            m_values[ch] = max;
            m_oldSign[ch] = -1;
            m_doneMap[ch] = false;
        }
    }
    this->writePar();

    if (m_rstPixelReg) {
        for (unsigned id=0; id<keeper->getNumOfEntries(); id++) {
            FrontEnd *fe = keeper->getEntry(id).fe;
            if (fe->getActive()) {
                Rd53b *rd53b = dynamic_cast<Rd53b*>(fe);
                g_tx->setCmdEnable(dynamic_cast<FrontEndCfg*>(fe)->getTxChannel());
                for (unsigned row=0; row<Rd53b::n_Row; row++) {
                    for (unsigned col=0; col<Rd53b::n_Col; col++) {
                        rd53b->setTDAC(col, row, m_pixelReg);
                    }
                }
            }
            while(!g_tx->isCmdEmpty()){}
        }
    }
    g_tx->setCmdEnable(keeper->getTxMask());
}

void Rd53bGlobalFeedback::execPart1() {
    g_stat->set(this, m_cur);
}

void Rd53bGlobalFeedback::execPart2() {
    // Wait for mutexes to be unlocked by feedback
    for (unsigned id=0; id<keeper->getNumOfEntries(); id++) {
        FrontEnd *fe = keeper->getEntry(id).fe;
        if (fe->getActive()) {
            unsigned rx = dynamic_cast<FrontEndCfg*>(fe)->getRxChannel();

            waitForFeedback(rx);
            logger->info(" --> Received Feedback on Channel {} with value: {}", rx, m_values[rx]);
        }
    }
    m_cur++;
    this->writePar();
    m_done = this->allDone();
}

void Rd53bGlobalFeedback::end() {
    for (unsigned id=0; id<keeper->getNumOfEntries(); id++) {
        FrontEnd *fe = keeper->getEntry(id).fe;
        if (fe->getActive()) {
            unsigned rx = dynamic_cast<FrontEndCfg*>(fe)->getRxChannel();
            logger->info(" --> Final parameter for Channel {} is {}", rx, m_values[rx]);
        }
    }
    this->writePar();
}


