/*
 * Authors: T. Heim <timon.heim@cern.ch>,
 * Date: 2014-Oct-16
 */

#include "Fei4ParameterLoop.h"

Fei4ParameterLoop::Fei4ParameterLoop() {

}

void Fei4ParameterLoop::init() {
    m_done = false;
    cur = min;
    g_fe->writeRegister(par, min);
}

void Fei4ParameterLoop::end() {
   
}

void Fei4ParameterLoop::execPart1() {

}

void Fei4ParameterLoop::execPart2() {
    cur += step;
    g_fe->writeRegister(par, cur);
    if (cur > max) m_done = true;

}

void Fei4ParameterLoop::setRange(unsigned arg_min, unsigned arg_max, unsigned arg_step) {
    min = arg_min;
    max = arg_max;
    step = arg_step;
}
