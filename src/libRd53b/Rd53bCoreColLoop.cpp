// #################################
// # Author: Timon Heim
// # Email: timon.heim at cern.ch
// # Project: Yarr
// # Description: Loop over core columns for RD53B
// # Date: 07/2020
// ################################

#include "Rd53bCoreColLoop.h"

#include "Rd53b.h"
#include "logging.h"

namespace {
    auto logger = logging::make_log("Rd53bCoreColLoop");
}

Rd53bCoreColLoop::Rd53bCoreColLoop() : LoopActionBase(LOOP_STYLE_MASK){
    m_cur = 0;
    m_nSteps = 25;
    min = 0;
    max = 25;
    m_minCore = 0;
    m_maxCore = 50;
    step = 1;
    m_coreCols = {0x0, 0x0, 0x0, 0x0};
    loopType = typeid(this);
    m_done = false;
    m_usePToT = false;
    m_disUnused = false;
}

void Rd53bCoreColLoop::init() {
    SPDLOG_LOGGER_TRACE(logger, "");
    m_done = false;
    m_cur = 0;
}

void Rd53bCoreColLoop::execPart1() {
    
    //Disable everything
    m_coreCols = {0x0, 0x0, 0x0, 0x0};
    const uint32_t one = 0x1;
    for (unsigned i=m_minCore; i<m_maxCore; i++) {
        if (i%m_nSteps == m_cur) {
            m_coreCols[i/16] |= one << i%16;
        }
    }
    logger->debug("Core Col stage #{0} (0x{1:x}, 0x{2:x}, 0x{3:x}, 0x{4:x})", m_cur, m_coreCols[0], m_coreCols[1], m_coreCols[2], m_coreCols[3]);
    this->setCores();
    g_stat->set(this, m_cur);
}

void Rd53bCoreColLoop::execPart2() {
    SPDLOG_LOGGER_TRACE(logger, "");
    m_cur += step;
    if (!(m_cur < m_nSteps)) m_done = true;
}

void Rd53bCoreColLoop::end() {
    // TODO return to original config
}

void Rd53bCoreColLoop::writeConfig(json &j) {
    j["min"] = m_minCore;
    j["max"] = m_maxCore;
    j["step"] = step;
    j["nSteps"] = m_nSteps;
    j["usePToT"] = m_usePToT;
}

void Rd53bCoreColLoop::loadConfig(const json &j) {
    if (j.contains("min"))
        m_minCore = j["min"];
    if (j.contains("max"))
        m_maxCore = j["max"];
    if (j.contains("step"))
        step = j["step"];
    if (j.contains("nSteps"))
        m_nSteps = j["nSteps"];
    if (j.contains("usePToT"))
        m_usePToT = j["usePToT"];
    if (j.contains("disableUnused"))
        m_disUnused = j["disableUnused"];
    min = 0;
    max = m_nSteps;
    if (m_nSteps > (m_maxCore-m_minCore) )
	    logger->warn("The number of steps {} is larger than the numbner of cores {}-{}", m_nSteps, m_maxCore, m_minCore);
}

void Rd53bCoreColLoop::setCores() {
    g_tx->setCmdEnable(keeper->getTxMask());
    Rd53b *rd53b = dynamic_cast<Rd53b*>(g_fe);
    // When enabling/disabling large amount
    if (m_disUnused) {
        if (m_nSteps <= 5)
            logger->warn("Enabling/disabling large amounts of core columns could lead to data link instability!");
        rd53b->writeRegister(&Rd53b::EnCoreCol0, m_coreCols[0]);
        rd53b->writeRegister(&Rd53b::EnCoreCol1, m_coreCols[1]);
        rd53b->writeRegister(&Rd53b::EnCoreCol2, m_coreCols[2]);
        rd53b->writeRegister(&Rd53b::EnCoreCol3, m_coreCols[3]);
        while(!g_tx->isCmdEmpty()) {}
    }
    // Set correct reset path
    //rd53b->writeRegister(&Rd53b::RstCoreCol0, m_coreCols[0]);
    //rd53b->writeRegister(&Rd53b::RstCoreCol1, m_coreCols[1]);
    //rd53b->writeRegister(&Rd53b::RstCoreCol2, m_coreCols[2]);
    //rd53b->writeRegister(&Rd53b::RstCoreCol3, m_coreCols[3]);
    //while(!g_tx->isCmdEmpty()) {}
    // Enable injection to core
    rd53b->writeRegister(&Rd53b::EnCoreColCal0, m_coreCols[0]);
    rd53b->writeRegister(&Rd53b::EnCoreColCal1, m_coreCols[1]);
    rd53b->writeRegister(&Rd53b::EnCoreColCal2, m_coreCols[2]);
    rd53b->writeRegister(&Rd53b::EnCoreColCal3, m_coreCols[3]);
    while(!g_tx->isCmdEmpty()) {}
	// Enable hitORs
    rd53b->writeRegister(&Rd53b::HitOrMask0, ~m_coreCols[0]);
    rd53b->writeRegister(&Rd53b::HitOrMask1, ~m_coreCols[1]);
    rd53b->writeRegister(&Rd53b::HitOrMask2, ~m_coreCols[2]);
    rd53b->writeRegister(&Rd53b::HitOrMask3, ~m_coreCols[3]);	
    while(!g_tx->isCmdEmpty()) {}
    // Turn on PToT if needed
    if (m_usePToT) {
        rd53b->writeRegister(&Rd53b::PtotCoreColEn0, m_coreCols[0]);
        rd53b->writeRegister(&Rd53b::PtotCoreColEn1, m_coreCols[1]);
        rd53b->writeRegister(&Rd53b::PtotCoreColEn2, m_coreCols[2]);
        rd53b->writeRegister(&Rd53b::PtotCoreColEn3, m_coreCols[3]);
    }
    while(!g_tx->isCmdEmpty()) {}
}

