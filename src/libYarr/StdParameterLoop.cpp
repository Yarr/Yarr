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
    m_checkActiveLoop = false;
}

void StdParameterLoop::init() {
    m_done = false;
    m_cur = min;
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
      this->writePar();
    }
}

void StdParameterLoop::end() {
    // Reset to min
    m_cur = min;
    this->writePar();
}

void StdParameterLoop::writePar() {
    if (!m_checkActiveLoop){
        SPDLOG_LOGGER_DEBUG(spllog, "Writing to global FE for parameter {}", parName);
        keeper->getGlobalFe()->writeNamedRegister(parName, m_cur);
        if (m_secondaryParName != ""){
            SPDLOG_LOGGER_DEBUG(spllog, "Writing to global FE for secondary parameter {}, set value to {}", m_secondaryParName, m_cur + m_secondaryOffset);
            keeper->getGlobalFe()->writeNamedRegister(m_secondaryParName, m_cur + m_secondaryOffset);
        }
    }
    else {
        SPDLOG_LOGGER_DEBUG(spllog, "Looping over FEs for parameter {}", parName);
        for (unsigned id=0; id<keeper->getNumOfEntries(); id++) {
            SPDLOG_LOGGER_DEBUG(spllog, "FE # {}", id);
            auto fe = keeper->getFe(id);
            if(fe->isActiveLoop()) {
                SPDLOG_LOGGER_DEBUG(spllog, "FE is active, writing to it");
                fe->writeNamedRegister(parName, m_cur);
            }
            if (m_secondaryParName != ""){
                SPDLOG_LOGGER_DEBUG(spllog, "Writing to active FE for secondary parameter {}, set value to {}", m_secondaryParName, m_cur + m_secondaryOffset);
                fe->writeNamedRegister(m_secondaryParName, m_cur + m_secondaryOffset);
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
    j["checkActiveLoop"] = m_checkActiveLoop;
    j["secondary"] = {
        {"parameter", m_secondaryParName},
        {"offset", m_secondaryOffset}
    };
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
    if (j.contains("checkActiveLoop"))
        m_checkActiveLoop = j["checkActiveLoop"];
    if (j.contains("secondary")){
        m_secondaryParName = j["secondary"]["parameter"];
        m_secondaryOffset = j["secondary"]["offset"];
    }
}
