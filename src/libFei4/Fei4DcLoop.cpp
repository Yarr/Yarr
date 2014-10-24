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
            m_colEnd = 40;
            break;
        case QUAD_DC:
            m_colEnd = 4;
            break;
        case OCTA_DC:
            m_colEnd = 8;
            break;
        case ALL_DC:
            m_colEnd = 1;
            break;
        default:
            break;
    }
    // Set COLPR_MODE
    g_fe->writeRegister((&Fei4::Colpr_Mode), (uint16_t) m_mode);
    m_col = 0;
}

void Fei4DcLoop::end() {
    if (verbose)
        std::cout << __PRETTY_FUNCTION__ << std::endl;
}

void Fei4DcLoop::execPart1() {
    if (verbose)
        std::cout << __PRETTY_FUNCTION__ << " --> " << m_col << std::endl;
    if (splitData)
        g_stat->set(this, m_col);
    // Address col
    g_fe->writeRegister(&Fei4::Colpr_Addr, m_col);
}

void Fei4DcLoop::execPart2() {
    if (verbose)
        std::cout << __PRETTY_FUNCTION__ << std::endl;
    // Check Loop condition
    m_col++;
    if (!(m_col < m_colEnd)) m_done = true;
}

void Fei4DcLoop::setMode(enum DC_MODE mode) {
    m_mode = mode;
}

enum DC_MODE Fei4DcLoop::getMode() {
    return m_mode;
}


