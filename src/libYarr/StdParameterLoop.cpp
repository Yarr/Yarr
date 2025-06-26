// #################################
// # Author: Bruce Gallop
// # Project: Yarr
// # Description: Generic Named Parameter Loop
// ################################

#include "StdParameterLoop.h"

#include <iostream>

#include "Bookkeeper.h"
#include "TxCore.h"

#include "logging.h"

namespace {
    auto spllog = logging::make_log("StdParameterLoop");
}

StdParameterLoop::StdParameterLoop() : LoopActionBase(LOOP_STYLE_PARAMETER) {
    loopType = typeid(this);
    min = 0;
    max = 100;
    step = 1;
    m_waitTime = std::chrono::microseconds(0);
    m_cur = 0;
    m_FEloop = false;
}

void StdParameterLoop::init() {
    m_done = false;
    m_cur = min;
    SPDLOG_LOGGER_DEBUG(spllog, "In StdParameterLoop::init for {} at -> {}", parName, m_cur);
    this->writePar();
}

void StdParameterLoop::execPart1() {
    SPDLOG_LOGGER_DEBUG(spllog, "ParameterLoop for {} at -> {}", parName, m_cur);
    g_stat->set(this, m_cur);
}

void StdParameterLoop::execPart2() {
    m_cur += step;
    if ((int)m_cur > max) {
      m_done = true;
    } else {
      SPDLOG_LOGGER_DEBUG(spllog, "In StdParameterLoop::execPart2 for {} at -> {}", parName, m_cur);
      this->writePar();
    }
}

void StdParameterLoop::end() {
    // Reset to min
    m_cur = min;
    this->writePar();
}

void StdParameterLoop::writePar() {
    if (!m_FEloop){
        SPDLOG_LOGGER_DEBUG(spllog, "Writing to global FE");
        keeper->getGlobalFe()->writeNamedRegister(parName, m_cur);
    }
    else {
        SPDLOG_LOGGER_DEBUG(spllog, "Looping over FEs");
        for (unsigned id=0; id<keeper->getNumOfEntries(); id++) {
            SPDLOG_LOGGER_DEBUG(spllog, "FE # {}", id);
            auto fe = keeper->getFe(id);
            if(fe->isActive()) {
                SPDLOG_LOGGER_DEBUG(spllog, "FE is active, writing to it");
                fe->writeNamedRegister(parName, m_cur);
            }
        }
    }
    while(!g_tx->isCmdEmpty());
    // Wait for potential stabilisation
    if (m_waitTime.count() > 0)
        std::this_thread::sleep_for(m_waitTime);
}

void StdParameterLoop::writeConfig(json &j) {
    j["min"] = min;
    j["max"] = max;
    j["step"] = step;
    j["parameter"] = parName;
    j["waitTime"] = m_waitTime.count();
    j["FEloop"] = m_FEloop;
}

void StdParameterLoop::loadConfig(const json &j) {
    if (j.contains("min"))
        min = j["min"];
    if (j.contains("max"))
        max = j["max"];
    if (j.contains("step"))
        step = j["step"];
    if (j.contains("parameter")) {
        SPDLOG_LOGGER_DEBUG(spllog, "Linking parameter: {}", std::string(j["parameter"]));
        parName = j["parameter"];
    }
    if (j.contains("waitTime")) {
        m_waitTime = std::chrono::microseconds(j["waitTime"]);
    }
    if (j.contains("FEloop"))
        m_FEloop = j["FEloop"];
}
