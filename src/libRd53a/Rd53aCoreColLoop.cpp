// #################################
// # Author: Timon Heim
// # Email: timon.heim at cern.ch
// # Project: Yarr
// # Description: Loop over core columns for RD53A
// # Date: 03/2018
// ################################

#include "Rd53aCoreColLoop.h"
#include "FrontEnd.h"
#include "Rd53a.h"

#include "logging.h"

namespace {
  auto logger = logging::make_log("Rd53aCoreColLoop");
}

class Rd53aCoreColLoop::Impl {
    public:
    unsigned m_cur;
    unsigned nSteps;
    unsigned maxCore;
    unsigned minCore;
};


Rd53aCoreColLoop::Rd53aCoreColLoop()
  : LoopActionBase(LOOP_STYLE_MASK), m_impl( new Rd53aCoreColLoop::Impl() )
{
    min = 0;
    max = 50;
    m_impl->nSteps = 25;
    step = 1;
    m_impl->m_cur = 0;
    loopType = typeid(this);
    m_done = false;
}

void Rd53aCoreColLoop::init() {
    SPDLOG_LOGGER_TRACE(logger, "");
    m_done = false;
    m_impl->m_cur = 0;
    // Disable all to begin with
    g_tx->setCmdEnable(keeper->getTxMask());
    auto rd53a = dynamic_cast<Rd53a*>(g_fe);
    // Loop over cores, i.e. activate in pairs of 4 DC
    for (unsigned dc=0; dc<Rd53a::n_DC; dc+=4) {
        rd53a->disableCalCol(dc);
        rd53a->disableCalCol(dc+1);
        rd53a->disableCalCol(dc+2);
        rd53a->disableCalCol(dc+3);
    }
    while(!g_tx->isCmdEmpty()) {}
}

void Rd53aCoreColLoop::execPart1() {
    SPDLOG_LOGGER_TRACE(logger, "{}", m_impl->m_cur);
    
    g_tx->setCmdEnable(keeper->getTxMask());

    auto rd53a = dynamic_cast<Rd53a*>(g_fe);

    // Loop over cores, i.e. activate in pairs of 4 DC
    for (unsigned dc=(m_impl->minCore*4), i=0; dc<(m_impl->maxCore*4); dc+=4, i++) {
        // Disable previous columns
        if (m_impl->m_cur>0 && ((i%m_impl->nSteps) == (m_impl->m_cur-step))) {
            rd53a->disableCalCol(dc);
            rd53a->disableCalCol(dc+1);
            rd53a->disableCalCol(dc+2);
            rd53a->disableCalCol(dc+3);
        }
        // Enable next columns
        if (i%m_impl->nSteps == m_impl->m_cur) {
            logger->debug("Enabling QC -> {}", dc);
            rd53a->enableCalCol(dc);
            rd53a->enableCalCol(dc+1);
            rd53a->enableCalCol(dc+2);
            rd53a->enableCalCol(dc+3);
        }
    //Add fine delay
        while(!g_tx->isCmdEmpty()) {}
    }

    // TODO this needs to be changed to be per FE
    if ( m_delayArray.size() > 0 ) {
        if ( m_delayArray.size() == (m_impl->maxCore-m_impl->minCore) ) 
            rd53a->writeRegister(&Rd53a::InjDelay,m_delayArray[m_impl->m_cur]);
        else 
            rd53a->writeRegister(&Rd53a::InjDelay,m_delayArray[0]);
    }
    while(!g_tx->isCmdEmpty()) {}
    
    g_stat->set(this, m_impl->m_cur);
    //std::this_thread::sleep_for(std::chrono::milliseconds(20));
}

void Rd53aCoreColLoop::execPart2() {
    SPDLOG_LOGGER_TRACE(logger, "");
    m_impl->m_cur += step;
    if (!(m_impl->m_cur < m_impl->nSteps)) m_done = true;
    // Nothing else to do here?
    
}

void Rd53aCoreColLoop::end() {
    SPDLOG_LOGGER_TRACE(logger, "");
    
    // TODO should restore original config here
    /*
    for(unsigned id=0; id<keeper->getNumOfEntries(); id++) {
        auto fe = keeper->getFe(id);
        // Loop over cores, i.e. activate in pairs of 4 DC
        for (unsigned dc=0; dc<Rd53a::n_DC; dc+=4) {
            dynamic_cast<Rd53a*>(fe)->enableCalCol(dc);
            dynamic_cast<Rd53a*>(fe)->enableCalCol(dc+1);
            dynamic_cast<Rd53a*>(fe)->enableCalCol(dc+2);
            dynamic_cast<Rd53a*>(fe)->enableCalCol(dc+3);
        }
    }
    while(!g_tx->isCmdEmpty()) {}
    */
    //std::this_thread::sleep_for(std::chrono::milliseconds(20));
}

void Rd53aCoreColLoop::writeConfig(json &j) {
    j["min"] = m_impl->minCore;
    j["max"] = m_impl->maxCore;
    j["step"] = step;
    j["nSteps"] = m_impl->nSteps;
    j["delayArray"] = m_delayArray;
}

void Rd53aCoreColLoop::loadConfig(const json &j) {
    if (j.contains("min"))
        m_impl->minCore = j["min"];
    if (j.contains("max"))
        m_impl->maxCore = j["max"];
    if (j.contains("step"))
        step = j["step"];
    if (j.contains("nSteps"))
        m_impl->nSteps = j["nSteps"];
    min = 0;
    max = m_impl->nSteps;
    if (j.contains("delayArray")) {
        m_delayArray.clear();
        for(auto i: j["delayArray"])
            m_delayArray.push_back(i);
        logger->debug("Number of injection delay array elements is {}", m_delayArray.size());
    }
    // Fine delay scan check
    if (m_impl->nSteps != (m_impl->maxCore-m_impl->minCore) )
	    logger->warn("The number of steps {} is different from {}", m_impl->nSteps, m_impl->maxCore-m_impl->minCore);
    else if ( m_delayArray.size() != m_impl->nSteps )
	    logger->warn("Fine delay array size is not matching the number of injected columns, only the first fine delay number will be used!!!");
}
