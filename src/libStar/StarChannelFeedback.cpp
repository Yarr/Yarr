// #################################
// # Author:
// # Email:
// # Project: Yarr
// # Description: StarChips Channel Feedback Loop action
// # Comment: 
// # Date: April 2018
// ################################

#include "StarChannelFeedback.h"
#include "logging.h"

namespace {
    auto logger = logging::make_log("StarChannelFeedback");
}

StarChannelFeedback::StarChannelFeedback() : LoopActionBase(LOOP_STYLE_PIXEL_FEEDBACK) {
    min = -15;
    max = 15;
    step = 1;
    m_cur = 0;
    loopType = typeid(this);
    m_done = false;
    m_resetTdac = true;
}

void StarChannelFeedback::writeConfig(json &j) {
    j["min"] = min;
    j["max"] = max;
    j["steps"] = m_steps;
    j["resetTdac"] = m_resetTdac;
}

void StarChannelFeedback::loadConfig(const json &j) {
    if (j.contains("min"))
        min = j["min"];
    if (j.contains("max"))
        max = j["max"];
    if (j.contains("resetTdac"))
        m_resetTdac = j["resetTdac"];
    if (j.contains("steps")) {
        m_steps.clear();
        for(auto i: j["steps"])
            m_steps.push_back(i);
    }
}

void StarChannelFeedback::feedback(unsigned channel, std::unique_ptr<Histo2d> h) {
    StarChips* fe = (StarChips*) keeper->getFe(channel);
    int nRow = fe->geo.nRow;
    int nCol = fe->geo.nCol;
    // TODO Check on NULL pointer
    if (h->size() != nRow*nCol) {
        logger->error("Wrong type of feedback histogram on channel {}.", channel);
        doneMap[channel] = true;
    } else {
        m_fb[channel] = std::move(h);
        for (unsigned row=1; row<=nRow; row++) {
            for (unsigned col=1; col<=nCol; col++) {
                int sign = m_fb[channel]->getBin(m_fb[channel]->binNum(col, row));

                //getTrimDAC and setTrimDAC use an old histogram layout converting here for now
                int v = fe->getTrimDAC(col, row);
                logger->trace("row {}, col {}, v {}, sign {}",row,col,v,sign);

                v = v + ((m_steps[m_cur])*sign);
                if (v<min) v = min;
                if (v>max) v = max;
                fe->setTrimDAC(col, row, v);
            }
        }
    }
}

void StarChannelFeedback::writeChannelCfg(StarChips *fe) {
    g_tx->setCmdEnable(dynamic_cast<FrontEndCfg*>(fe)->getTxChannel());
    fe->writeTrims();
    while(!g_tx->isCmdEmpty());
    g_tx->setCmdEnable(keeper->getTxMask());
}

void StarChannelFeedback::init() {
    m_done = false;
    m_cur = 0;
    // Init maps
    if (m_resetTdac) {
        for (auto *fe : keeper->feList) {
            if (fe->getActive()) {
            	int nRow = fe->geo.nRow;
            	int nCol = fe->geo.nCol; 
                unsigned ch = dynamic_cast<FrontEndCfg*>(fe)->getRxChannel();
                m_fb[ch] = NULL;
                for (unsigned row=1; row<=nRow; row++) {
                    for (unsigned col=1; col<=nCol; col++) {                        
                        //Initial TDAC in mid of the range
                        dynamic_cast<StarChips*>(fe)->setTrimDAC(col, row, 15);
                    }
                }
            }
        }
    }
}

void StarChannelFeedback::execPart1() {
    g_stat->set(this, m_cur);
    // Lock all mutexes
    for (auto fe : keeper->feList) {
        if (fe->getActive()) {
            this->writeChannelCfg(dynamic_cast<StarChips*>(fe));
        }
    }
}

void StarChannelFeedback::execPart2() {
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

void StarChannelFeedback::end() {
    
    for (auto fe: keeper->feList) {
        if (fe->getActive()) {
            this->writeChannelCfg(dynamic_cast<StarChips*>(fe));
        }
    }
    
}
