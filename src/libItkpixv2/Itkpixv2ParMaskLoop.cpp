// #################################
// # Author: Timon Heim
// # Email: timon.heim at cern.ch
// # Project: Yarr
// # Description: Parallel Mask Loop for ITkPix, fixed mask pattern
// # Date: 07/2023
// ################################

#include "Itkpixv2ParMaskLoop.h"

#include "logging.h"

namespace {
  auto logger = logging::make_log("Itkpixv2ParMaskLoop");
}

Itkpixv2ParMaskLoop::Itkpixv2ParMaskLoop() : LoopActionBase(LOOP_STYLE_MASK) {
    min = 0;
    max = 64;
    step = 1;
    m_cur = 0;
    loopType = typeid(this);
    m_done = false;
}

void Itkpixv2ParMaskLoop::init() {
    SPDLOG_LOGGER_TRACE(logger, "");
    m_done = false;
    m_cur = min;
    
    g_tx->setCmdEnable(keeper->getTxMask());
    Itkpixv2 *itkpix = dynamic_cast<Itkpixv2*>(g_fe);
    
    // Turn off all pixels to start with
    for (unsigned col=0; col<8; col++) { // Just need first core col
        for (unsigned row=0; row<Itkpixv2::n_Row; row++) {
            itkpix->setEn(col, row, 0);
            itkpix->setInjEn(col, row, 0);
            itkpix->setHitbus(col, row, 0);
        }
    }
    
    itkpix->configurePixelMaskParallel();
    while(!g_tx->isCmdEmpty()) {}
}

void Itkpixv2ParMaskLoop::execPart1() {
    SPDLOG_LOGGER_TRACE(logger, "");

    g_tx->setCmdEnable(keeper->getTxMask());
    Itkpixv2 *itkpix = dynamic_cast<Itkpixv2*>(g_fe);
    unsigned counter = 0;
        
    for(unsigned col=0; col<8; col++) {
        for(unsigned row=0; row<Itkpixv2::n_Row; row++) {
            // Disable pixels of last mask stage
            if (itkpix->getInjEn(col, row) == 1) {
                itkpix->setEn(col, row, 0);
                itkpix->setInjEn(col, row, 0);
                itkpix->setHitbus(col, row, 0);
            }		
            // Enable pixels of current mask stage
            if (applyMask(col,row)){
                itkpix->setEn(col, row, 1);
                itkpix->setInjEn(col, row, 1);
                itkpix->setHitbus(col, row, 1);
                counter++;
            }
        }
        
    }
    itkpix->configurePixelMaskParallel();
    while(!g_tx->isCmdEmpty()) {}
    
    g_tx->setCmdEnable(keeper->getTxMask());
    g_stat->set(this, m_cur);
    logger->info(" ---> Mask Stage {} (Activated {} pixels in one Core Column)", m_cur, counter);
}

void Itkpixv2ParMaskLoop::execPart2() {
    SPDLOG_LOGGER_TRACE(logger, "");
    m_cur += step;
    if (!((int)m_cur < max)) m_done = true;
}

void Itkpixv2ParMaskLoop::end() {

}

bool Itkpixv2ParMaskLoop::applyMask(unsigned col, unsigned row) {
    // This is the mask pattern
    unsigned core_row = row/8;
    unsigned serial;
    serial = (core_row*64)+((col+(core_row%8))%8)*8+row%8;
    //unsigned serial = (col%8*Itkpixv2::n_Row)+row;
    if ((serial%max) == m_cur){
        return true;
    }
    return false;
}

void Itkpixv2ParMaskLoop::writeConfig(json &j) {
    j["min"] = min;
    j["max"] = max;
    j["step"] = step;
}

void Itkpixv2ParMaskLoop::loadConfig(const json &j) {
    if (j.contains("min"))
        min = j["min"];
    if (j.contains("max"))
        max = j["max"];
    if (j.contains("step"))
        step = j["step"];
}
