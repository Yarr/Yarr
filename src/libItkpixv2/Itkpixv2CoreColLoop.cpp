// #################################
// # Author: Timon Heim
// # Email: timon.heim at cern.ch
// # Project: Yarr
// # Description: Loop over core columns for ITkPixV2
// # Date: 07/2023
// ################################

#include "Itkpixv2CoreColLoop.h"

#include "Bookkeeper.h"
#include "Itkpixv2.h"
#include "logging.h"

namespace {
    auto logger = logging::make_log("Itkpixv2CoreColLoop");
}

Itkpixv2CoreColLoop::Itkpixv2CoreColLoop() : LoopActionBase(LOOP_STYLE_MASK){
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
    m_ignoreDis = false;
}

void Itkpixv2CoreColLoop::init() {
    SPDLOG_LOGGER_TRACE(logger, "");
    m_done = false;
    m_cur = 0;

    if(m_ignoreDis){
        m_initCoreColsAllChips.clear();
        Itkpixv2Cfg *m_feCfg;
        std::array<uint16_t, 4> m_initCoreColsSingle = {0x0, 0x0, 0x0, 0x0};
        for (unsigned id =0; id<keeper->getNumOfEntries(); id++){
            auto fe = keeper->getFe(id);
            if (fe->getActive()){
                auto m_feCfg = dynamic_cast<Itkpixv2Cfg*>(fe);
                m_initCoreColsSingle[0] = m_feCfg->EnCoreCol0.read();
                m_initCoreColsSingle[1] = m_feCfg->EnCoreCol1.read();
                m_initCoreColsSingle[2] = m_feCfg->EnCoreCol2.read();
                m_initCoreColsSingle[3] = m_feCfg->EnCoreCol3.read();
                m_initCoreColsAllChips.push_back(m_initCoreColsSingle);
            }
        }

        int iChannel =0;
        for (auto channel: keeper->getTxMask()) {
            for (int iReg=0; iReg<4; iReg++) {	
                logger->debug("Initially, for channel {} EnCoreCol{} set to {} ",iChannel,iReg,m_initCoreColsAllChips[iChannel][iReg]);
            }
            iChannel++;
        }
    }
}

void Itkpixv2CoreColLoop::execPart1() {

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

void Itkpixv2CoreColLoop::execPart2() {
    SPDLOG_LOGGER_TRACE(logger, "");
    m_cur += step;
    if (!(m_cur < m_nSteps)) m_done = true;

    //Issue reset
    if (m_resetAtEnd){
        g_tx->writeFifo(0xAAAA0000 |Itkpixv2Cmd::genClear(16)[0]);
        std::this_thread::sleep_for(std::chrono::microseconds(10));
        g_rx->flushBuffer();
    }

}

void Itkpixv2CoreColLoop::end() {
    // TODO return to original config

    // When in core column test, set all to enable
    if (m_disUnused && m_ignoreDis){
        int iChannel=0;
        for (unsigned id =0; id<keeper->getNumOfEntries(); id++){
            auto fe = keeper->getFe(id);
            if (!(fe->getActive()))
                continue;

            std::array<int,4> allOn = {65535,65535,65535,63};
            auto m_feCfg = dynamic_cast<Itkpixv2Cfg*>(fe);
            g_tx->setCmdEnable(m_feCfg->getTxChannel());
            for (int iReg=0; iReg<4; iReg++){
                uint16_t toSet = allOn[iReg];
                std::string registerName = "EnCoreCol"+std::to_string(iReg);
                fe->writeNamedRegister(registerName,toSet);
            }
            iChannel++;
            while(!g_tx->isCmdEmpty()) {}

        }
    }
}

void Itkpixv2CoreColLoop::writeConfig(json &j) {
    j["min"] = m_minCore;
    j["max"] = m_maxCore;
    j["step"] = step;
    j["nSteps"] = m_nSteps;
    j["usePToT"] = m_usePToT;
    j["resetAtEnd"] = m_resetAtEnd;
    j["ignoreDisabled"] = m_ignoreDis;
}

void Itkpixv2CoreColLoop::loadConfig(const json &j) {
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
    if (j.contains("resetAtEnd"))
        m_resetAtEnd = j["resetAtEnd"];
    if (j.contains("ignoreDisabled"))
        m_ignoreDis = j["ignoreDisabled"];
    min = 0;
    max = m_nSteps;
    if (m_nSteps > (m_maxCore-m_minCore) )
        logger->warn("The number of steps {} is larger than the numbner of cores {}-{}", m_nSteps, m_maxCore, m_minCore);
}

void Itkpixv2CoreColLoop::setCores() {
    g_tx->setCmdEnable(keeper->getTxMask());
    Itkpixv2 *itkpixv2 = dynamic_cast<Itkpixv2*>(g_fe);
    // When enabling/disabling large amount
    if (m_disUnused) {
        if (m_nSteps <= 5)
            logger->warn("Enabling/disabling large amounts of core columns could lead to data link instability!");
        itkpixv2->writeRegister(&Itkpixv2::EnCoreCol0, m_coreCols[0]);
        itkpixv2->writeRegister(&Itkpixv2::EnCoreCol1, m_coreCols[1]);
        itkpixv2->writeRegister(&Itkpixv2::EnCoreCol2, m_coreCols[2]);
        itkpixv2->writeRegister(&Itkpixv2::EnCoreCol3, m_coreCols[3]);
        while(!g_tx->isCmdEmpty()) {}
    }
    //Turn off columns that were originally off in chip config
    //set cmd disable
    if (m_disUnused and m_ignoreDis){
        int iChannel=0;
        for (unsigned id =0; id<keeper->getNumOfEntries(); id++){
            auto fe = keeper->getFe(id);
            if (!(fe->getActive()))
                continue;

            auto m_feCfg = dynamic_cast<Itkpixv2Cfg*>(fe);
            g_tx->setCmdEnable(m_feCfg->getTxChannel());
            for (int iReg=0; iReg<4; iReg++){
                uint16_t toSet = m_initCoreColsAllChips[iChannel][iReg] & m_coreCols[iReg];
                std::string registerName = "EnCoreCol"+std::to_string(iReg);
                fe->writeNamedRegister(registerName,toSet);
            }
            iChannel++;
            while(!g_tx->isCmdEmpty()) {}

        }
    }

    g_tx->setCmdEnable(keeper->getTxMask());
    // Set correct reset path
    //itkpixv2->writeRegister(&Itkpixv2::RstCoreCol0, m_coreCols[0]);
    //itkpixv2->writeRegister(&Itkpixv2::RstCoreCol1, m_coreCols[1]);
    //itkpixv2->writeRegister(&Itkpixv2::RstCoreCol2, m_coreCols[2]);
    //itkpixv2->writeRegister(&Itkpixv2::RstCoreCol3, m_coreCols[3]);
    //while(!g_tx->isCmdEmpty()) {}
    // Enable injection to core
    itkpixv2->writeRegister(&Itkpixv2::EnCoreColCal0, m_coreCols[0]);
    itkpixv2->writeRegister(&Itkpixv2::EnCoreColCal1, m_coreCols[1]);
    itkpixv2->writeRegister(&Itkpixv2::EnCoreColCal2, m_coreCols[2]);
    itkpixv2->writeRegister(&Itkpixv2::EnCoreColCal3, m_coreCols[3]);
    while(!g_tx->isCmdEmpty()) {}
    // Enable hitORs
    itkpixv2->writeRegister(&Itkpixv2::HitOrMask0, ~m_coreCols[0]);
    itkpixv2->writeRegister(&Itkpixv2::HitOrMask1, ~m_coreCols[1]);
    itkpixv2->writeRegister(&Itkpixv2::HitOrMask2, ~m_coreCols[2]);
    itkpixv2->writeRegister(&Itkpixv2::HitOrMask3, ~m_coreCols[3]);	
    while(!g_tx->isCmdEmpty()) {}
    // Turn on PToT if needed
    if (m_usePToT) {
        itkpixv2->writeRegister(&Itkpixv2::PtotCoreColEn0, m_coreCols[0]);
        itkpixv2->writeRegister(&Itkpixv2::PtotCoreColEn1, m_coreCols[1]);
        itkpixv2->writeRegister(&Itkpixv2::PtotCoreColEn2, m_coreCols[2]);
        itkpixv2->writeRegister(&Itkpixv2::PtotCoreColEn3, m_coreCols[3]);
    }
    while(!g_tx->isCmdEmpty()) {}
}

