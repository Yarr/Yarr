// #################################
// # Author: Timon Heim
// # Email: timon.heim at cern.ch
// # Project: Yarr
// # Description: Mask Loop for RD53B
// # Date: 07/2020
// ################################

#include "Rd53bMaskLoop.h"

#include "logging.h"

namespace {
  auto logger = logging::make_log("Rd53bMaskLoop");
}

Rd53bMaskLoop::Rd53bMaskLoop() : LoopActionBase(LOOP_STYLE_MASK) {
    min = 0;
    max = 64;
    step = 1;
    m_cur = 0;
    loopType = typeid(this);
    m_done = false;
}

void Rd53bMaskLoop::init() {
    SPDLOG_LOGGER_TRACE(logger, "");
    m_done = false;
    m_cur = min;
    
    for (FrontEnd *fe : keeper->feList) {
        Rd53b *rd53b = dynamic_cast<Rd53b*>(fe);
        g_tx->setCmdEnable(dynamic_cast<FrontEndCfg*>(fe)->getTxChannel());
        // Save current version of the pix regs to transferred back into the config at the end
        m_pixRegs[fe] = rd53b->pixRegs;
        // Turn off all pixels to start with
        for (unsigned col=0; col<Rd53b::n_Col; col++) {
            for (unsigned row=0; row<Rd53b::n_Row; row++) {
                rd53b->setEn(col, row, 0); // TODO make configurable
                rd53b->setInjEn(col, row, 0);
            }
        }
        rd53b->configurePixels();
        while(!g_tx->isCmdEmpty()) {}
    }
    g_tx->setCmdEnable(keeper->getTxMask());
}

void Rd53bMaskLoop::execPart1() {
    SPDLOG_LOGGER_TRACE(logger, "");

    unsigned counter = 0;
    for(FrontEnd *fe : keeper->feList) {
        g_tx->setCmdEnable(dynamic_cast<FrontEndCfg*>(fe)->getTxChannel());
        std::vector<std::pair<unsigned, unsigned>> modPixels;

        Rd53b *rd53b = dynamic_cast<Rd53b*>(fe);
        
        for(unsigned col=0; col<Rd53b::n_Col; col++) {
            for(unsigned row=0; row<Rd53b::n_Row; row++) {
                // Disable pixels of last mask stage
                if (rd53b->getInjEn(col, row) == 1) {
                    //logger->info("Disabling {};{}", col, row);
                    rd53b->setEn(col, row, 0); // TODO make configurable
                    rd53b->setInjEn(col, row, 0);
                    modPixels.push_back(std::make_pair(col, row));
                }		
                // Enable pixels of current mask stage
                if (applyMask(col,row)){
                    //logger->info("Enabling {};{}", col, row);
                    rd53b->setEn(col, row, 1); // TODO Make configurable
                    rd53b->setInjEn(col, row, 1);
                    modPixels.push_back(std::make_pair(col, row));
                    counter++;
                }
            }
        }
        
        rd53b->configurePixels();
        while(!g_tx->isCmdEmpty()) {}
    }
    
    g_tx->setCmdEnable(keeper->getTxMask());
    g_stat->set(this, m_cur);
    logger->info(" ---> Mask Stage {} (Activated {} pixels)", m_cur, counter);
}

void Rd53bMaskLoop::execPart2() {
    SPDLOG_LOGGER_TRACE(logger, "");
    m_cur += step;
    if (!((int)m_cur < max)) m_done = true;
}

void Rd53bMaskLoop::end() {
    for(FrontEnd *fe : keeper->feList) {
        // Copy original registers back
        // TODO need to make sure analysis modifies the right config
        // TODO not thread safe, in case analysis modifies them to early
        dynamic_cast<Rd53b*>(fe)->pixRegs = m_pixRegs[fe];
    }
}

bool Rd53bMaskLoop::applyMask(unsigned col, unsigned row) {
    // This is the mask pattern
    unsigned core_row = row/8;
    unsigned serial = (core_row*64)+((col+(core_row%8))%8)*8+row%8;
    //unsigned serial = (col%8*Rd53b::n_Row)+row;
    if ((serial%max) == m_cur){
        return true;
    }
    return false;
}

void Rd53bMaskLoop::writeConfig(json &j) {
    j["min"] = min;
    j["max"] = max;
    j["step"] = step;
}

void Rd53bMaskLoop::loadConfig(json &j) {
    if (!j["min"].empty())
        min = j["min"];
    if (!j["max"].empty())
        max = j["max"];
    if (!j["step"].empty())
        step = j["step"];
}
