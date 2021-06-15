// #################################
// # Author:
// # Email:
// # Project: Yarr
// # Description: StarChips Channel Feedback Loop action
// # Comment: 
// # Date: April 2018
// ################################

#include "include/StarChannelFeedback.h"
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
    tuneLin = true;
    m_resetTdac = true;
}

void StarChannelFeedback::writeConfig(json &j) {
    j["min"] = min;
    j["max"] = max;
    j["steps"] = m_steps;
    j["tuneLin"] = tuneLin;
    j["resetTdac"] = m_resetTdac;
}

void StarChannelFeedback::loadConfig(json &j) {
    if (!j["min"].empty())
        min = j["min"];
    if (!j["max"].empty())
        max = j["max"];
    if (!j["tuneLin"].empty())
        tuneLin = j["tuneLin"];
    if (!j["resetTdac"].empty())
        m_resetTdac = j["resetTdac"];
    if (!j["steps"].empty()) {
        m_steps.clear();
        for(auto i: j["steps"])
            m_steps.push_back(i);
        std::cout << "Got " << m_steps.size() << " steps!!" << std::endl;
    }
}

void StarChannelFeedback::feedback(unsigned channel, std::unique_ptr<Histo2d> h) {
	std::cout << __PRETTY_FUNCTION__ << "   " << h->getName() <<std::endl;
    // TODO Check on NULL pointer
    if (h->size() != m_nRow*m_nCol) {
        std::cout << __PRETTY_FUNCTION__ 
            << " --> ERROR : Wrong type of feedback histogram on channel " << channel << std::endl;
        doneMap[channel] = true;
    } else {
        m_fb[channel] = std::move(h);
        for (unsigned row=1; row<=m_nRow; row++) {
            for (unsigned col=1; col<=m_nCol; col++) {
                int sign = m_fb[channel]->getBin(m_fb[channel]->binNum(col, row));

                //getTrimDAC and setTrimDAC use an old histogram layout converting here for now
                unsigned row_alt = row % 128;
                unsigned col_alt = col + 2*(row >> 7);

                int v = dynamic_cast<StarChips*>(keeper->getFe(channel))->getTrimDAC(col_alt, row_alt);

                v = v + ((m_steps[m_cur])*sign);
                if (v<min) v = min;
                if (v>max) v = max;
                dynamic_cast<StarChips*>(keeper->getFe(channel))->setTrimDAC(col_alt, row_alt, v);
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
    if (1)
        std::cout << __PRETTY_FUNCTION__ << std::endl;
    m_done = false;
    m_cur = 0;
    // Init maps
    if (m_resetTdac) {
        for (auto *fe : keeper->feList) {
            if (fe->getActive()) {
            	m_nRow = fe->geo.nRow;
            	m_nCol = fe->geo.nCol; 
                unsigned ch = dynamic_cast<FrontEndCfg*>(fe)->getRxChannel();
                m_fb[ch] = NULL;
                for (unsigned row=1; row<=m_nRow; row++) {
                	for (unsigned col=1; col<=m_nCol; col++) {
                        //Initial TDAC in mid of the range
                    	dynamic_cast<StarChips*>(keeper->getFe(ch))->setTrimDAC(col, row, 15);
                    }
                }
            }
        }
    }
}

void StarChannelFeedback::execPart1() {
	std::cout << __PRETTY_FUNCTION__ << std::endl;
    g_stat->set(this, m_cur);
    // Lock all mutexes
    for (auto fe : keeper->feList) {
        if (fe->getActive()) {
            this->writeChannelCfg(dynamic_cast<StarChips*>(fe));
        }
    }
    std::cout << " -> Feedback step " << m_cur << " with size " << m_steps[m_cur] << std::endl;
}

void StarChannelFeedback::execPart2() {
	std::cout << __PRETTY_FUNCTION__ << std::endl;
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
