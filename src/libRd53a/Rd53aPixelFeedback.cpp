// #################################
// # Author: Timon Heim
// # Email: timon.heim at cern.ch
// # Project: Yarr
// # Description: RD53A Global Feedback Loopaction
// # Comment: 
// # Date: April 2018
// ################################

#include "Rd53aPixelFeedback.h"

#include "logging.h"

namespace {
  auto logger = logging::make_log("Rd53aPixelFeedback");
}

Rd53aPixelFeedback::Rd53aPixelFeedback() : LoopActionBase(LOOP_STYLE_PIXEL_FEEDBACK) {
    min = -15;
    max = 15;
    step = 1;
    m_cur = 0;
    loopType = typeid(this);
    m_done = false;
    tuneLin = true;
    tuneDiff = true;
    m_resetTdac = true;
}

void Rd53aPixelFeedback::writeConfig(json &j) {
    j["min"] = min;
    j["max"] = max;
    j["steps"] = m_steps;
    j["tuneDiff"] = tuneDiff;
    j["tuneLin"] = tuneLin;
    j["resetTdac"] = m_resetTdac;
}

void Rd53aPixelFeedback::loadConfig(const json &j) {
    if (j.contains("min"))
        min = j["min"];
    if (j.contains("max"))
        max = j["max"];
    if (j.contains("tuneDiff"))
        tuneDiff = j["tuneDiff"];
    if (j.contains("tuneLin"))
        tuneLin = j["tuneLin"];
    if (j.contains("resetTdac"))
        m_resetTdac = j["resetTdac"];
    if (j.contains("steps")) {
        m_steps.clear();
        for(auto i: j["steps"])
            m_steps.push_back(i);
        logger->debug("Got {} steps!", m_steps.size());
    }
}

void Rd53aPixelFeedback::feedback(unsigned id, std::unique_ptr<Histo2d> h) {
    // TODO Check on NULL pointer
    if (h->size() != Rd53a::n_Row*Rd53a::n_Col) {
        logger->error("Wrong type of feedback histogram on channel {}", id);
        fbDoneMap[id] = true;
    } else {
        auto rd53a = dynamic_cast<Rd53a*>(keeper->getFe(id));
        for (unsigned row=1; row<=Rd53a::n_Row; row++) {
            for (unsigned col=1; col<=Rd53a::n_Col; col++) {
                int sign = h->getBin(h->binNum(col, row));
                int v = rd53a->getTDAC(col-1, row-1);
                if (128<col && col<=264 && tuneLin) {
                    v = v + ((m_steps[m_cur])*sign);
                    if (v<min) v = min;
                    if (v>max) v = max;
                } else if (264<col && tuneDiff) {
                    v = v + (m_steps[m_cur]*sign*-1);
                    if (v<min) v = min;
                    if (v>max) v = max;
                }
                rd53a->setTDAC(col-1, row-1, v);
            }
        }
        m_fb[id] = std::move(h);
    }
}

void Rd53aPixelFeedback::writePixelCfg(Rd53a *fe) {
    g_tx->setCmdEnable(dynamic_cast<FrontEndCfg*>(fe)->getTxChannel());
    fe->configurePixels();
    while(!g_tx->isCmdEmpty());
    g_tx->setCmdEnable(keeper->getTxMask());
}

void Rd53aPixelFeedback::init() {
    SPDLOG_LOGGER_TRACE(logger, "");
    m_done = false;
    m_cur = 0;
    // Init maps
    for(unsigned id=0; id<keeper->getNumOfEntries(); id++) {
        fbDoneMap[id] = true;
    }
    if (m_resetTdac) {
        for(unsigned id=0; id<keeper->getNumOfEntries(); id++) {
            auto fe = keeper->getFe(id);
            if (fe->getActive()) {
                auto rd53a = dynamic_cast<Rd53a*>(fe);
                m_fb[id] = NULL;
                int linCnt = 0;
                int diffCnt = 0;
                for (unsigned col=1; col<=Rd53a::n_Col; col++) {
                    for (unsigned row=1; row<=Rd53a::n_Row; row++) {
                        //Initial TDAC in mid of the range
                        if (128<col && col<=264 && tuneLin) {
                            rd53a->setTDAC(col-1, row-1, 8);
                            linCnt++;
                        } else if (264<col && tuneDiff) {
                            rd53a->setTDAC(col-1, row-1, 0);
                            diffCnt++;
                        }
                    }
                }
            }
        }
    }
}

void Rd53aPixelFeedback::execPart1() {
    g_stat->set(this, m_cur);
    // Lock all mutexes
    for(unsigned id=0; id<keeper->getNumOfEntries(); id++) {
        auto fe = keeper->getFe(id);
        if (fe->getActive()) {
            this->writePixelCfg(dynamic_cast<Rd53a*>(fe));
        }
    }
    logger->info(" -> Feedback step #{} of {} with size {}", m_cur+1, m_steps.size(), m_steps[m_cur]);
}

void Rd53aPixelFeedback::execPart2() {
    // Wait for mutexes to be unlocked by feedback
    for(unsigned id=0; id<keeper->getNumOfEntries(); id++) {
        auto fe = keeper->getFe(id);
        if (fe->getActive()) {
            waitForFeedback(id);
        }
    }
    m_cur++;
    if (m_cur == m_steps.size()) {
        m_done = true;
    } else if(isFeedbackDone()) {
        logger->error("Wrong type of feedback histogram on all channels");
        m_done = true;
    }

}

void Rd53aPixelFeedback::end() {
    /*
    for(unsigned id=0; id<keeper->getNumOfEntries(); id++) {
        auto fe = keeper->getFe(id);
            keeper->mutexMap[id].lock();
            this->writePixelCfg(dynamic_cast<Rd53a*>(fe));
        }
    }
    */
}
