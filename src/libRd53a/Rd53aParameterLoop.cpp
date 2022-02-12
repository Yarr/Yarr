// #################################
// # Author: Timon Heim
// # Email: timon.heim at cern.ch
// # Project: Yarr
// # Description: Parameter Loop for RD53A
// # Date: 03/2018
// ################################

#include "Rd53aParameterLoop.h"

#include "logging.h"

namespace {
  auto logger = logging::make_log("Rd53aParameterLoop");
}

Rd53aParameterLoop::Rd53aParameterLoop() : LoopActionBase(LOOP_STYLE_PARAMETER) {
    loopType = typeid(this);
    min = 0;
    max = 100;
    step = 1;

}

Rd53aParameterLoop::Rd53aParameterLoop(Rd53aReg Rd53aGlobalCfg::*ref) : LoopActionBase(LOOP_STYLE_PARAMETER), parPtr(ref) {
    loopType = typeid(this);
    min = 0;
    max = 100;
    step = 1;
    //TODO parName not set

}

void Rd53aParameterLoop::init() {
    SPDLOG_LOGGER_TRACE(logger, "");

    m_done = false;
    m_cur = min;
    parPtr = keeper->globalFe<Rd53a>()->regMap[parName];
    this->writePar();
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
}

void Rd53aParameterLoop::execPart1() {
    SPDLOG_LOGGER_TRACE(logger, "");
    logger->debug("ParameterLoop at -> {}", m_cur);
    g_stat->set(this, m_cur);
    //std::this_thread::sleep_for(std::chrono::milliseconds(10));
}

void Rd53aParameterLoop::execPart2() {
    m_cur += step;
    if ((int)m_cur > max) m_done = true;
    this->writePar();
}

void Rd53aParameterLoop::end() {
    // Reset to min
    m_cur = min;
    this->writePar();
}

void Rd53aParameterLoop::writePar() {
    keeper->globalFe<Rd53a>()->writeRegister(parPtr, m_cur);
    while(!g_tx->isCmdEmpty());
    //std::this_thread::sleep_for(std::chrono::milliseconds(20));
}

void Rd53aParameterLoop::writeConfig(json &j) {
    j["min"] = min;
    j["max"] = max;
    j["step"] = step;
    j["parameter"] = parName;
}

void Rd53aParameterLoop::loadConfig(const json &j) {
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
