// #################################
// # Author: Timon Heim
// # Email: timon.heim at cern.ch
// # Project: Yarr
// # Description: Parameter Loop for ITkPixV2
// # Date: 07/2023
// ################################

#include "Itkpixv2ParameterLoop.h"

#include "logging.h"

namespace {
    auto logger = logging::make_log("Itkpixv2ParameterLoop");
}

Itkpixv2ParameterLoop::Itkpixv2ParameterLoop() : LoopActionBase(LOOP_STYLE_PARAMETER) {
    loopType = typeid(this);
    min = 0;
    max = 100;
    step = 1;

}

Itkpixv2ParameterLoop::Itkpixv2ParameterLoop(Itkpixv2RegDefault Itkpixv2GlobalCfg::*ref) : LoopActionBase(LOOP_STYLE_PARAMETER), parPtr(ref) {
    loopType = typeid(this);
    min = 0;
    max = 100;
    step = 1;
    //TODO parName not set

}

void Itkpixv2ParameterLoop::init() {
    SPDLOG_LOGGER_TRACE(logger, "");

    m_done = false;
    m_cur = min;
    parPtr = keeper->globalFe<Itkpixv2>()->getNamedRegister(parName);
    this->writePar();
    //std::this_thread::sleep_for(std::chrono::milliseconds(10));
}

void Itkpixv2ParameterLoop::execPart1() {
    SPDLOG_LOGGER_TRACE(logger, "");
    logger->debug("ParameterLoop at -> {}", m_cur);
    g_stat->set(this, m_cur);
    //std::this_thread::sleep_for(std::chrono::milliseconds(10));
}

void Itkpixv2ParameterLoop::execPart2() {
    m_cur += step;
    if ((int)m_cur > max) m_done = true;
    this->writePar();
}

void Itkpixv2ParameterLoop::end() {
    // Reset to min
    m_cur = min;
    this->writePar();
}

void Itkpixv2ParameterLoop::writePar() {
    keeper->globalFe<Itkpixv2>()->writeRegister(parPtr, m_cur);
    while(!g_tx->isCmdEmpty());
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
}

void Itkpixv2ParameterLoop::writeConfig(json &j) {
    j["min"] = min;
    j["max"] = max;
    j["step"] = step;
    j["parameter"] = parName;
}

void Itkpixv2ParameterLoop::loadConfig(const json &j) {
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
