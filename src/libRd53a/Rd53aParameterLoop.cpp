// #################################
// # Author: Timon Heim
// # Email: timon.heim at cern.ch
// # Project: Yarr
// # Description: Parameter Loop for RD53A
// # Date: 03/2018
// ################################

#include "Rd53aParameterLoop.h"

Rd53aParameterLoop::Rd53aParameterLoop() {
    loopType = typeid(this);
    min = 0;
    max = 100;
    step = 1;

}

Rd53aParameterLoop::Rd53aParameterLoop(Rd53aReg Rd53aGlobalCfg::*ref): parPtr(ref) {
    loopType = typeid(this);
    min = 0;
    max = 100;
    step = 1;
    //TODO parName not set

}

void Rd53aParameterLoop::init() {
    if (verbose)
        std::cout << __PRETTY_FUNCTION__ << std::endl;
    m_done = false;
    m_cur = min;
    parPtr = keeper->globalFe<Rd53a>()->regMap[parName];
    this->writePar();
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
}

void Rd53aParameterLoop::execPart1() {
    if (verbose)
        std::cout << " : ParameterLoop at -> " << m_cur << std::endl;
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

void Rd53aParameterLoop::loadConfig(json &j) {
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
