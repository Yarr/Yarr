/*
 * Authors: T. Heim <timon.heim@cern.ch>
 * Date: 2016-May-16
 */

#include "Fe65p2ParameterLoop.h"

Fe65p2ParameterLoop::Fe65p2ParameterLoop(Fe65p2GlobalReg Fe65p2GlobalCfg::*reg) {
    m_reg = reg;
    cur = 0;
    loopType = typeid(this);
    m_done = false;
}

void Fe65p2ParameterLoop::setRange(unsigned arg_min, unsigned arg_max, unsigned arg_step) {
    min = arg_min;
    max = arg_max;
    step = arg_step;
}

void Fe65p2ParameterLoop::init() {
    cur = min;
    m_done = false;
}

void Fe65p2ParameterLoop::end() {

}

void Fe65p2ParameterLoop::execPart1() {
   g_fe65p2->setValue(m_reg, (uint16_t) cur);
   //std::cout << " Par = " << cur << std::endl;
   g_fe65p2->configureGlobal();
   g_fe65p2->configDac();
   g_stat->set(this, cur);
}

void Fe65p2ParameterLoop::execPart2() {
    cur += step;
    if (cur > max) m_done = true;
}
