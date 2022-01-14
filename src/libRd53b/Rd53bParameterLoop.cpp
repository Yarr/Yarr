// #################################
// # Author: Timon Heim
// # Email: timon.heim at cern.ch
// # Project: Yarr
// # Description: Parameter Loop for RD53B
// # Date: 07/2020
// ################################

#include "Rd53bParameterLoop.h"

#include "logging.h"

namespace {
    auto logger = logging::make_log("Rd53bParameterLoop");
}

Rd53bParameterLoop::Rd53bParameterLoop() : LoopActionBase(LOOP_STYLE_PARAMETER) {
    loopType = typeid(this);
    min = 0;
    max = 100;
    step = 1;

}

Rd53bParameterLoop::Rd53bParameterLoop(Rd53bReg Rd53bGlobalCfg::*ref) : LoopActionBase(LOOP_STYLE_PARAMETER), parPtr(ref) {
    loopType = typeid(this);
    min = 0;
    max = 100;
    step = 1;
    //TODO parName not set

}

void Rd53bParameterLoop::init() {
    SPDLOG_LOGGER_TRACE(logger, "");

    m_done = false;
    m_cur = min;
    parPtr = keeper->globalFe<Rd53b>()->getNamedRegister(parName);
    this->writePar();
    //std::this_thread::sleep_for(std::chrono::milliseconds(10));
}

void Rd53bParameterLoop::execPart1() {
    SPDLOG_LOGGER_TRACE(logger, "");
    logger->debug("ParameterLoop at -> {}", m_cur);
    g_stat->set(this, m_cur);
    //std::this_thread::sleep_for(std::chrono::milliseconds(10));
}

void Rd53bParameterLoop::execPart2() {
    m_cur += step;
    if ((int)m_cur > max) m_done = true;
    this->writePar();
}

void Rd53bParameterLoop::end() {
    // Reset to min
    m_cur = min;
    this->writePar();
}

void Rd53bParameterLoop::writePar() {
    keeper->globalFe<Rd53b>()->writeRegister(parPtr, m_cur);
    while(!g_tx->isCmdEmpty());
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
}

void Rd53bParameterLoop::writeConfig(json &j) {
    j["min"] = min;
    j["max"] = max;
    j["step"] = step;
    j["parameter"] = parName;
}

void Rd53bParameterLoop::loadConfig(const json &j) {
    if (j.contains("min"))
        min = j["min"];
    if (j.contains("max"))
        max = j["max"];
    if (j.contains("step"))
        step = j["step"];
    if (j.contains("parameter")) {
        logger->info("Linking parameter: {}", std::string(j["parameter"]));
        parName = j["parameter"];
    }
}
