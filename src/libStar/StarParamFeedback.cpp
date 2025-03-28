// #################################
// # Project: Yarr
// # Description: StarChips feedback of analysis parameter
// ################################

#include "StarParamFeedback.h"
#include "logging.h"

namespace {
    auto logger = logging::make_log("StarParamFeedback");
}

StarParamFeedback::StarParamFeedback() : LoopActionBase(LOOP_STYLE_PIXEL_FEEDBACK) {
    min = 0;
    max = 0;
    step = 1;
    m_cur = 0;
    loopType = typeid(this);
    m_done = false;
}

void StarParamFeedback::writeConfig(json &j) {
    j["parameter"] = m_par;
}

void StarParamFeedback::loadConfig(const json &j) {
    if (j.contains("parameter"))
        m_par = j["parameter"];
}

// Response to message sent from Analysis (via waitForFeedback)
void StarParamFeedback::feedback(unsigned id, std::unique_ptr<Histo2d> h) {
    if(fbDoneMap[id]) {
        logger->warn("Refuse to receive feedback twice {}", id);
        return;
    }

    logger->trace("Received feedback for ID {}", id);
    auto fe = (StarChips*) keeper->getFe(id);

    if(m_par != "STR_DEL") {
        logger->warn("StarParamFeedback currently only implemented for strobe delay (STR_DEL) not {}", m_par);
        return;
    }

    unsigned nCol = fe->geo.nCol;
    unsigned nABCs = nCol / 128;

    if(h->getXbins() != nABCs) {
      logger->warn("Expecting {} bins to match chips, got {}",
                   nABCs, h->getXbins());
      return;
    }

    for (unsigned histo_abc=0; histo_abc<nABCs; histo_abc++) {
        int bin = histo_abc; // Skip underflow

        int new_sd = h->getBin(bin);

        if(!fe->isAbcForHistoChip(histo_abc)) {
            logger->warn("Failed to find chip for feedback {} {}", histo_abc, new_sd);

            fe->logMappings();

            // Skip to the next chip
            continue;
        }

        AbcCfg &abc = fe->abcForHistoChip(histo_abc);

        logger->trace("Loading feedback at {} (ID {}) {}", histo_abc, abc.getABCchipID(), new_sd);

        abc.setSubRegisterValue("STR_DEL", new_sd);
        logger->trace(" Check load {}", abc.getSubRegisterValue("STR_DEL"));
    }
    fbDoneMap[id] = true;
}

void StarParamFeedback::writeChannelCfg(StarChips *fe) {
    g_tx->setCmdEnable(dynamic_cast<FrontEndCfg*>(fe)->getTxChannel());
    while(!g_tx->isCmdEmpty());
    g_tx->setCmdEnable(keeper->getTxMask());
}

void StarParamFeedback::init() {
    logger->trace("Init");
    m_done = false;
    m_cur = 0;
}

// Before, just let the scan run
void StarParamFeedback::execPart1() {
    logger->trace("Start loop");
    g_stat->set(this, m_cur);
}

// After scan loop
void StarParamFeedback::execPart2() {
    logger->trace("End of loop wait for feedback");
    // Scan has run, now wait for feedback from analysis
    for (unsigned id=0; id<keeper->getNumOfEntries(); id++) {
        auto fe = keeper->getFe(id);
        if (fe->getActive()) {
            logger->trace("Waiting for feedback for ID {} at {} ", id, m_cur);
            waitForFeedback(id);
        } else {
            logger->trace("Skipping feedback as not active ID {}", id);
        }
    }
    m_cur++;

    if(!isFeedbackDone()) {
        logger->error("Feedback not received from all frontends");
    }

    logger->trace("Parameter {} feedback complete", m_par);

    // Only one iteration allowed
    m_done = true;
}

void StarParamFeedback::end() {
    logger->trace("End");

    for (unsigned id=0; id<keeper->getNumOfEntries(); id++) {
        auto fe = keeper->getFe(id);
        if (fe->getActive()) {
            this->writeChannelCfg(dynamic_cast<StarChips*>(fe));
        }
    }
}
