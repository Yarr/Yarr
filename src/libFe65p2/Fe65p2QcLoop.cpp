/*
 * Authors: T. Heim <timon.heim@cern.ch>,
 * Date: 2016-Mar-30
 */

#include "Fe65p2QcLoop.h"

Fe65p2QcLoop::Fe65p2QcLoop() {
    m_mask = 0x1111;
    min = 0;
    max = 4;
    step = 1;
    m_cur = 0;
    
    loopType = typeid(this);
}

void Fe65p2QcLoop::init() {

}

void Fe65p2QcLoop::end() {

}

void Fe65p2QcLoop::execPart1() {
    // All in parallel
    g_fe65p2->setValue(&Fe65p2::ColEn, (m_mask << m_cur));
    g_fe65p2->setValue(&Fe65p2::ColSrEn, (m_mask << m_cur));
    // Write to global regs
    g_fe65p2->configureGlobal();
}

void Fe65p2QcLoop::execPart2() {
    if (!(m_cur < max)) m_done = true;
}

