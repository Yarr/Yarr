/*
 * Authors: T. Heim <timon.heim@cern.ch>,
 * Date: 2013-Oct-22
 */

#include "Fei4DcLoop.h"

Fei4DcLoop::Fei4DcLoop() : LoopActionBase() {
    m_mode = SINGLE_DC;
    m_colStart = 0;
    m_colEnd = 40;
    m_col = 0;
}

void Fei4DcLoop::init() {
    if (verbose)
        std::cout << __PRETTY_FUNCTION__ << std::endl;
    // Figure out how often to loop
    // depending on COLPR_MODE
    switch (m_mode) {
        case SINGLE_DC:
            m_colEnd = 40;
            break;
        case QUAD_DC:
            m_colEnd = 10;
            break;
        case OCTA_DC:
            m_colEnd = 5;
            break;
        case ALL_DC:
            m_colEnd = 1;
            break;
        default:
            break;
    }
    // Set COLPR_MODE
    g_fe->writeRegister((&Fei4GlobalCfg::Colpr_Mode), (uint16_t) m_mode);
}

void Fei4DcLoop::end() {
    if (verbose)
        std::cout << __PRETTY_FUNCTION__ << std::endl;
}

void Fei4DcLoop::execPart1() {
    if (verbose)
        std::cout << __PRETTY_FUNCTION__ << std::endl;
    // Address col
    g_fe->writeRegister(&Fei4GlobalCfg::Colpr_Addr, m_col);
}

void Fei4DcLoop::execPart2() {
    if (verbose)
        std::cout << __PRETTY_FUNCTION__ << std::endl;
    // Check Loop condition
    m_col++;
    if (m_col >= m_colEnd) m_done = true;
}

void Fei4DcLoop::setMode(DcLoop_Type mode) {
    m_mode = mode;
}

DcLoop_Type Fei4DcLoop::getMode() {
    return m_mode;
}


