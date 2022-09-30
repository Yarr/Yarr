// #################################
// # Author: Timon Heim
// # Email: timon.heim at cern.ch
// # Project: Yarr
// # Description: Parallel Mask Loop for RD53B, fixed mask pattern
// # Date: 07/2022
// ################################

#include "Rd53bParMaskLoop.h"

#include "logging.h"

namespace {
  auto logger = logging::make_log("Rd53bParMaskLoop");
}

Rd53bParMaskLoop::Rd53bParMaskLoop() : LoopActionBase(LOOP_STYLE_MASK) {
    min = 0;
    max = 64;
    step = 1;
    m_cur = 0;
    loopType = typeid(this);
    m_done = false;
}

void Rd53bParMaskLoop::init() {
    SPDLOG_LOGGER_TRACE(logger, "");
    m_done = false;
    m_cur = min;
    
    g_tx->setCmdEnable(keeper->getTxMask());
    Rd53b *rd53b = dynamic_cast<Rd53b*>(g_fe);
    
    // Turn off all pixels to start with
    for (unsigned col=0; col<8; col++) { // Just need first core col
        for (unsigned row=0; row<Rd53b::n_Row; row++) {
            rd53b->setEn(col, row, 0);
            rd53b->setInjEn(col, row, 0);
            rd53b->setHitbus(col, row, 0);
        }
    }
    
    rd53b->configurePixelMaskParallel();
    while(!g_tx->isCmdEmpty()) {}
}

void Rd53bParMaskLoop::execPart1() {
    SPDLOG_LOGGER_TRACE(logger, "");

    g_tx->setCmdEnable(keeper->getTxMask());
    Rd53b *rd53b = dynamic_cast<Rd53b*>(g_fe);
    unsigned counter = 0;
        
    for(unsigned col=0; col<8; col++) {
        for(unsigned row=0; row<Rd53b::n_Row; row++) {
            // Disable pixels of last mask stage
            if (rd53b->getInjEn(col, row) == 1) {
                rd53b->setEn(col, row, 0);
                rd53b->setInjEn(col, row, 0);
                rd53b->setHitbus(col, row, 0);
            }		
            // Enable pixels of current mask stage
            if (applyMask(col,row)){
                rd53b->setEn(col, row, 1);
                rd53b->setInjEn(col, row, 1);
                rd53b->setHitbus(col, row, 1);
                counter++;
            }
        }
        
    }
    rd53b->configurePixelMaskParallel();
    while(!g_tx->isCmdEmpty()) {}
    
    g_tx->setCmdEnable(keeper->getTxMask());
    g_stat->set(this, m_cur);
    logger->info(" ---> Mask Stage {} (Activated {} pixels)", m_cur, counter);
}

void Rd53bParMaskLoop::execPart2() {
    SPDLOG_LOGGER_TRACE(logger, "");
    m_cur += step;
    if (!((int)m_cur < max)) m_done = true;
}

void Rd53bParMaskLoop::end() {

}

bool Rd53bParMaskLoop::applyMask(unsigned col, unsigned row) {
    // This is the mask pattern
    unsigned core_row = row/8;
    unsigned serial;
    serial = (core_row*64)+((col+(core_row%8))%8)*8+row%8;
    //unsigned serial = (col%8*Rd53b::n_Row)+row;
    if ((serial%max) == m_cur){
        return true;
    }
    return false;
}

void Rd53bParMaskLoop::writeConfig(json &j) {
    j["min"] = min;
    j["max"] = max;
    j["step"] = step;
}

void Rd53bParMaskLoop::loadConfig(const json &j) {
    if (j.contains("min"))
        min = j["min"];
    if (j.contains("max"))
        max = j["max"];
    if (j.contains("step"))
        step = j["step"];
}
