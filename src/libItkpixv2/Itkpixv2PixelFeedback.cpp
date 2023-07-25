// #################################
// # Author: Timon Heim
// # Email: timon.heim at cern.ch
// # Project: Yarr
// # Description: ITkPixV2 Pixel Feedback Loopaction
// # Date: 07/2023
// ################################

#include "Itkpixv2PixelFeedback.h"

#include "logging.h"

namespace {
    auto logger = logging::make_log("Itkpixv2PixelFeedback");
}

Itkpixv2PixelFeedback::Itkpixv2PixelFeedback() : LoopActionBase(LOOP_STYLE_PIXEL_FEEDBACK) {
    min = -15;
    max = 15;
    step = 1;
    m_cur = 0;
    loopType = typeid(this);
    m_done = false;
    m_rstPixelReg = true;
    m_pixelReg = 0;
}

void Itkpixv2PixelFeedback::writeConfig(json &j) {
    j["min"] = min;
    j["max"] = max;
    j["steps"] = m_steps;
    j["pixelReg"] = m_pixelReg;
    j["rstPixelReg"] = m_rstPixelReg;
}

void Itkpixv2PixelFeedback::loadConfig(const json &j) {
    if (j.contains("min"))
        min = j["min"];
    if (j.contains("max"))
        max = j["max"];
    if (j.contains("pixelReg"))
        m_pixelReg = j["pixelReg"];
    if (j.contains("rstPixelReg"))
        m_rstPixelReg = j["rstPixelReg"];
    if (j.contains("steps")) {
        m_steps.clear();
        for(auto i: j["steps"])
            m_steps.push_back(i);
        logger->debug("Got {} steps!", m_steps.size());
    }
}

void Itkpixv2PixelFeedback::feedback(unsigned id, std::unique_ptr<Histo2d> h) {
    // TODO Check on NULL pointer
    if (h->size() != Itkpixv2::n_Row*Itkpixv2::n_Col) {
        logger->error("Wrong type of feedback histogram on ID {}", id);
        fbDoneMap[id] = true;
    } else {
        auto itkpix = dynamic_cast<Itkpixv2*>(keeper->getFe(id));
        for (unsigned row=1; row<=Itkpixv2::n_Row; row++) {
            for (unsigned col=1; col<=Itkpixv2::n_Col; col++) {
                int sign = h->getBin(h->binNum(col, row));
                int v = itkpix->getTDAC(col-1, row-1);
                
                v = v + ((int)m_steps[m_cur]*sign*-1);
                if (v<min) v = min;
                if (v>max) v = max;
                
                itkpix->setTDAC(col-1, row-1, v);
            }
        }
        m_fb[id] = std::move(h);
    }
}

void Itkpixv2PixelFeedback::writePixelCfg(Itkpixv2 *fe) {
    g_tx->setCmdEnable(dynamic_cast<FrontEndCfg*>(fe)->getTxChannel());
    fe->configurePixels();
    while(!g_tx->isCmdEmpty());
    g_tx->setCmdEnable(keeper->getTxMask());
}

void Itkpixv2PixelFeedback::init() {
    SPDLOG_LOGGER_TRACE(logger, "");
    m_done = false;
    m_cur = 0;
    // Init maps
    for (unsigned id=0; id<keeper->getNumOfEntries(); id++) {
        fbDoneMap[id] = false;
    }
    if (m_rstPixelReg) {
        for (unsigned id=0; id<keeper->getNumOfEntries(); id++) {
            FrontEnd *fe = keeper->getEntry(id).fe;
            if (fe->getActive()) {
                auto itkpix = dynamic_cast<Itkpixv2*>(fe);
                m_fb[id] = NULL;
                for (unsigned col=1; col<=Itkpixv2::n_Col; col++) {
                    for (unsigned row=1; row<=Itkpixv2::n_Row; row++) {
                        //Initial TDAC in mid of the range
                        itkpix->setTDAC(col-1, row-1, m_pixelReg);
                    }
                }
            }
        }
    }
}

void Itkpixv2PixelFeedback::execPart1() {
    g_stat->set(this, m_cur);
    // Lock all mutexes
    for (unsigned id=0; id<keeper->getNumOfEntries(); id++) {
        FrontEnd *fe = keeper->getEntry(id).fe;
        if (fe->getActive()) {
            this->writePixelCfg(dynamic_cast<Itkpixv2*>(fe));
        }
    }
    logger->info(" -> Feedback step #{} of {} with size {}", m_cur+1, m_steps.size(), m_steps[m_cur]);
}

void Itkpixv2PixelFeedback::execPart2() {
    // Wait for mutexes to be unlocked by feedback
    for (unsigned id=0; id<keeper->getNumOfEntries(); id++) {
        FrontEnd *fe = keeper->getEntry(id).fe;
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

void Itkpixv2PixelFeedback::end() {
    // Nothing to do
}
