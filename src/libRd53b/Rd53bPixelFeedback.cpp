// #################################
// # Author: Timon Heim
// # Email: timon.heim at cern.ch
// # Project: Yarr
// # Description: RD53B Pixel Feedback Loopaction
// # Date: 07/2020
// ################################

#include "Rd53bPixelFeedback.h"

#include "logging.h"

namespace {
    auto logger = logging::make_log("Rd53bPixelFeedback");
}

Rd53bPixelFeedback::Rd53bPixelFeedback() : LoopActionBase(LOOP_STYLE_PIXEL_FEEDBACK) {
    min = -15;
    max = 15;
    step = 1;
    m_cur = 0;
    loopType = typeid(this);
    m_done = false;
    m_resetTdac = true;
}

void Rd53bPixelFeedback::writeConfig(json &j) {
    j["min"] = min;
    j["max"] = max;
    j["steps"] = m_steps;
    j["resetTdac"] = m_resetTdac;
}

void Rd53bPixelFeedback::loadConfig(json &j) {
    if (!j["min"].empty())
        min = j["min"];
    if (!j["max"].empty())
        max = j["max"];
    if (!j["resetTdac"].empty())
        m_resetTdac = j["resetTdac"];
    if (!j["steps"].empty()) {
        m_steps.clear();
        for(auto i: j["steps"])
            m_steps.push_back(i);
        logger->debug("Got {} steps!", m_steps.size());
    }
}

void Rd53bPixelFeedback::feedback(unsigned channel, std::unique_ptr<Histo2d> h) {
    // TODO Check on NULL pointer
    if (h->size() != Rd53b::n_Row*Rd53b::n_Col) {
        logger->error("Wrong type of feedback histogram on channel {}", channel);
        doneMap[channel] = true;
    } else {
        auto rd53b = dynamic_cast<Rd53b*>(keeper->getFe(channel));
        for (unsigned row=1; row<=Rd53b::n_Row; row++) {
            for (unsigned col=1; col<=Rd53b::n_Col; col++) {
                int sign = h->getBin(h->binNum(col, row));
                int v = rd53b->getTDAC(col-1, row-1);
                
                v = v + ((int)m_steps[m_cur]*sign*-1);
                if (v<min) v = min;
                if (v>max) v = max;
                
                rd53b->setTDAC(col-1, row-1, v);
            }
        }
        m_fb[channel] = std::move(h);
    }
}

void Rd53bPixelFeedback::writePixelCfg(Rd53b *fe) {
    g_tx->setCmdEnable(dynamic_cast<FrontEndCfg*>(fe)->getTxChannel());
    fe->configurePixels();
    while(!g_tx->isCmdEmpty());
    g_tx->setCmdEnable(keeper->getTxMask());
}

void Rd53bPixelFeedback::init() {
    SPDLOG_LOGGER_TRACE(logger, "");
    m_done = false;
    m_cur = 0;
    // Init maps
    if (m_resetTdac) {
        for (auto *fe : keeper->feList) {
            if (fe->getActive()) {
                unsigned ch = dynamic_cast<FrontEndCfg*>(fe)->getRxChannel();
                auto rd53b = dynamic_cast<Rd53b*>(keeper->getFe(ch));
                m_fb[ch] = NULL;
                for (unsigned col=1; col<=Rd53b::n_Col; col++) {
                    for (unsigned row=1; row<=Rd53b::n_Row; row++) {
                        //Initial TDAC in mid of the range
                        rd53b->setTDAC(col-1, row-1, 0);
                    }
                }
            }
        }
    }
}

void Rd53bPixelFeedback::execPart1() {
    g_stat->set(this, m_cur);
    // Lock all mutexes
    for (auto fe : keeper->feList) {
        if (fe->getActive()) {
            this->writePixelCfg(dynamic_cast<Rd53b*>(fe));
        }
    }
    logger->info(" -> Feedback step #{} of {} with size {}", m_cur+1, m_steps.size(), m_steps[m_cur]);
}

void Rd53bPixelFeedback::execPart2() {
    // Wait for mutexes to be unlocked by feedback
    for (auto fe: keeper->feList) {
        if (fe->getActive()) {
            unsigned rx = dynamic_cast<FrontEndCfg*>(fe)->getRxChannel();
            waitForFeedback(rx);
        }
    }
    m_cur++;
    if (m_cur == m_steps.size()) {
        m_done = true;
    }
}

void Rd53bPixelFeedback::end() {
    // Nothing to do
}
