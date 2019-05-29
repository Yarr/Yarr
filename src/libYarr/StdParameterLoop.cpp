// #################################
// # Author: Bruce Gallop
// # Project: Yarr
// # Description: Generic Named Parameter Loop
// ################################

#include "StdParameterLoop.h"

#include <iostream>

StdParameterLoop::StdParameterLoop() {
    loopType = typeid(this);
    min = 0;
    max = 100;
    step = 1;
}

void StdParameterLoop::init() {
    m_done = false;
    m_cur = min;
    this->writePar();
}

void StdParameterLoop::execPart1() {
    if (verbose)
        std::cout << " : ParameterLoop at -> " << m_cur << std::endl;
    g_stat->set(this, m_cur);
}

void StdParameterLoop::execPart2() {
    m_cur += step;
    if ((int)m_cur > max) m_done = true;
    this->writePar();
}

void StdParameterLoop::end() {
    // Reset to min
    m_cur = min;
    this->writePar();
}

void StdParameterLoop::writePar() {
    keeper->getGlobalFe()->writeNamedRegister(parName, m_cur);

    while(!g_tx->isCmdEmpty());
}

void StdParameterLoop::writeConfig(json &j) {
    j["min"] = min;
    j["max"] = max;
    j["step"] = step;
    j["parameter"] = parName;
}

void StdParameterLoop::loadConfig(json &j) {
    if (!j["min"].empty())
        min = j["min"];
    if (!j["max"].empty())
        max = j["max"];
    if (!j["step"].empty())
        step = j["step"];
    if (!j["parameter"].empty()) {
        std::cout << "  Linking parameter: " << j["parameter"] <<std::endl;
        parName = j["parameter"];
    }
}
