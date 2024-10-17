// #################################
// # Author:
// # Email:
// # Project: Yarr
// # Description: StarChips Channel Feedback Loop action
// # Comment: 
// # Date: April 2018
// ################################

#include "StarChannelFeedback.h"
#include "StarConstants.h"

#include "Bookkeeper.h"
#include "TxCore.h"
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
        for(auto &i: j["steps"])
            m_steps.push_back(i);
    }
}

void StarChannelFeedback::feedback(unsigned id, std::unique_ptr<Histo2d> h) {
    auto fe = (StarChips*) keeper->getFe(id);
    unsigned nRow = fe->geo.nRow;
    unsigned nCol = fe->geo.nCol;
    // TODO Check on NULL pointer
    if (h->size() != nRow*nCol) {
        logger->error("Wrong type of feedback histogram for ID {}.", id);
        fbDoneMap[id] = true;
    } else {
        m_fb[id] = std::move(h);

        // TODO Not used, need to verify that it shouldn't be!
        // auto chip_map = fe->hcc().histoChipMap();
        unsigned nABCs = nCol / Star::StripsPerABCRow;

        for (unsigned histo_abc=0; histo_abc<nABCs; histo_abc++) {
            AbcCfg &abc = fe->abcForHistoChip(histo_abc);

            for (unsigned chan=0; chan<Star::StripsPerABCRow; chan++) {
                unsigned histo_col = 1 + histo_abc * Star::StripsPerABCRow + chan;
                for (unsigned row=1; row<=nRow; row++) {
                    uint8_t abc_chan = chan + ((row-1) * Star::StripsPerABCRow);

                    int sign = m_fb[id]->getBin(m_fb[id]->binNum(histo_col, row));

                    //getTrimDAC and setTrimDAC use an old histogram layout converting here for now

                    uint8_t trimOrder = abc.trimRegOrderFromChannel(abc_chan);

                    int v = abc.getTrimDACRaw(trimOrder);
                    logger->trace("row {}, col {}, trim {}, v {}, sign {}",row,histo_col, trimOrder, v,sign);

                    v = v + ((m_steps[m_cur])*sign);
                    if (v<min) v = min;
                    if (v>max) v = max;

                    abc.setTrimDACRaw(trimOrder, v);
                }
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
        for (unsigned id=0; id<keeper->getNumOfEntries(); id++) {
            auto fe = keeper->getFe(id);
            if (fe->getActive()) {
                m_fb[id] = nullptr;
                auto &star = *dynamic_cast<StarChips*>(fe);

                // Set initial TDAC in mid of the range
                star.eachAbc([&](auto &cfg) {
                    for (unsigned chan=0; chan<Star::StripsPerABC; chan++) {
                        cfg.setTrimDACRaw(chan, 15);
                    }
                });
            }
        }
    }
}

void StarChannelFeedback::execPart1() {
    g_stat->set(this, m_cur);
    // Lock all mutexes
    for (unsigned id=0; id<keeper->getNumOfEntries(); id++) {
        auto fe = keeper->getFe(id);
        if (fe->getActive()) {
            this->writeChannelCfg(dynamic_cast<StarChips*>(fe));
        }
    }
}

void StarChannelFeedback::execPart2() {
    // Wait for mutexes to be unlocked by feedback
    for (unsigned id=0; id<keeper->getNumOfEntries(); id++) {
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

void StarChannelFeedback::end() {
    
    for (unsigned id=0; id<keeper->getNumOfEntries(); id++) {
        auto fe = keeper->getFe(id);
        if (fe->getActive()) {
            this->writeChannelCfg(dynamic_cast<StarChips*>(fe));
        }
    }
    
}
