/*
 * Authors: T. Heim <timon.heim@cern.ch>,
 * Date: 2016-Mar-30
 */

#include "Fe65p2QcLoop.h"

Fe65p2QcLoop::Fe65p2QcLoop() {
    m_mask = 0x0101;
    min = 0;
    max = 8;
    step = 1;
    m_cur = 0;
    m_done = false;
    
    loopType = typeid(this);
}

void Fe65p2QcLoop::init() {
    m_cur = min;
    m_done = false;
}

void Fe65p2QcLoop::end() {
}

void Fe65p2QcLoop::execPart1() {
    std::cout << "\t--> Qc Loop: " << m_cur << std::endl;
    // All in parallel
    keeper->globalFe<Fe65p2>()->setValue(&Fe65p2::ColEn, (m_mask << m_cur));
    //keeper->globalFe<Fe65p2>()->setValue(&Fe65p2::ColSrEn, (m_mask << m_cur));
    // Write to global regs
    keeper->globalFe<Fe65p2>()->configureGlobal();
}

void Fe65p2QcLoop::execPart2() {
    m_cur+=step;
    if (!((int)m_cur < max)) m_done = true;
}

