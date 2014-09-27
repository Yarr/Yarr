/*
 * Authors: T. Heim <timon.heim@cern.ch>,
 * Date: 2013-Oct-22
 */

#include <StdTriggerLoop.h>

StdTriggerLoop::StdTriggerLoop() : LoopActionBase() {
    m_maxTrigCnt = 50; // Maximum numberof triggers to send
    m_curTrigCnt = 0; // Current number of triggers sent
    m_trigDelay = 20; // Delay between injection and trigger
}

void StdTriggerLoop::init() {
    // Setup FE into run mode
    g_fe->setRunMode(true);
}

void StdTriggerLoop::end() {
    g_fe->setRunMode(false);
}

void StdTriggerLoop::execPart1() {
    // Trigger
}

void StdTriggerLoop::execPart2() {
    // TODO Wait for data ?
    // Or just some time?

    // Check Loop condition
    m_curTrigCnt++;
    if (m_curTrigCnt >= m_maxTrigCnt) m_done = true;
}

void StdTriggerLoop::setMaxTrigCnt(unsigned int cnt) {
    m_maxTrigCnt = cnt;
}

unsigned int StdTriggerLoop::getMaxTrigCnt() {
    return m_maxTrigCnt;
}

void StdTriggerLoop::setTrigDelay(unsigned int delay) {
    m_trigDelay = delay;
}

unsigned int StdTriggerLoop::getTrigDelay() {
    return m_trigDelay;
}

