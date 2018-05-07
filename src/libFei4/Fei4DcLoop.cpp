/*
 * Authors: T. Heim <timon.heim@cern.ch>,
 * Date: 2013-Oct-22
 */

#include "Fei4DcLoop.h"

Fei4DcLoop::Fei4DcLoop() : LoopActionBase() {
    m_mode = SINGLE_DC;
    min = 0;
    max = 40;
    step = 1;
    m_col = 0;

    loopType = typeid(this);
}

void Fei4DcLoop::init() {
    m_done = false;
    if (verbose)
        std::cout << __PRETTY_FUNCTION__ << std::endl;
    // Figure out how often to loop
    // depending on COLPR_MODE
    switch (m_mode) {
        case SINGLE_DC:
            max = 40;
            break;
        case QUAD_DC:
            max = 4;
            break;
        case OCTA_DC:
            max = 8;
            break;
        case ALL_DC:
            max = 1;
            break;
        default:
            break;
    }
    // Set COLPR_MODE
    keeper->globalFe<Fei4>()->writeRegister((&Fei4::Colpr_Mode), (uint16_t) m_mode);
    m_col = min;
}

void Fei4DcLoop::end() {
    if (verbose)
        std::cout << __PRETTY_FUNCTION__ << std::endl;
}

void Fei4DcLoop::execPart1() {
    if (verbose)
        std::cout << __PRETTY_FUNCTION__ << " --> " << m_col << std::endl;
    g_stat->set(this, m_col);
    // Address col
    keeper->globalFe<Fei4>()->writeRegister(&Fei4::Colpr_Addr, m_col);
    while(!g_tx->isCmdEmpty());
}

void Fei4DcLoop::execPart2() {
    if (verbose)
        std::cout << __PRETTY_FUNCTION__ << std::endl;
    // Check Loop condition
    m_col+=step;
    if (!((int)m_col < max)) m_done = true;
}

void Fei4DcLoop::setMode(enum DC_MODE mode) {
    m_mode = mode;
    switch (m_mode) {
        case SINGLE_DC:
            max = 40;
            break;
        case QUAD_DC:
            max = 4;
            break;
        case OCTA_DC:
            max = 8;
            break;
        case ALL_DC:
            max = 1;
            break;
        default:
            break;
    }
}

uint32_t Fei4DcLoop::getMode() {
    return m_mode;
}

void Fei4DcLoop::writeConfig(json &config) {
    config["min"] = min;
    config["max"] = max;
    config["step"] = step;
    config["mode"] = m_mode;
}

void Fei4DcLoop::loadConfig(json &config) {
    min = config["min"];
    max = config["max"];
    step = config["step"];
    m_mode = (uint32_t) config["mode"];

}
