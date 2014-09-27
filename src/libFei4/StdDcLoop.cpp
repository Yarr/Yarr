/*
 * Authors: T. Heim <timon.heim@cern.ch>,
 * Date: 2013-Oct-22
 */

#include "StdDcLoop.h"

StdDcLoop::StdDcLoop() : LoopActionBase() {
    m_mode = SINGLE_DC;
    m_colStart = 0;
    m_colEnd = 40;
    m_col = 0;
}

void StdDcLoop::init() {
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
    //m_grp->setRegister((&Fei4GlobalCfg::CP), (uint16_t) m_mode);
}

void StdDcLoop::end() {
}

void StdDcLoop::execPart1() {
    // Address col
    //m_grp->setRegister(&Fei4GlobalCfg::Colpr_Addr, m_col);
}

void StdDcLoop::execPart2() {
    
    // Check Loop condition
    m_col++;
    if (m_col >= m_colEnd) m_done = true;
}

void StdDcLoop::setMode(DcLoop_Type mode) {
    m_mode = mode;
}

DcLoop_Type StdDcLoop::getMode() {
    return m_mode;
}


