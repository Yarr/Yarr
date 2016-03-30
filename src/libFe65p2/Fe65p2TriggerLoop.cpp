/*
 * Authors: T. Heim <timon.heim@cern.ch>
 * Date: 2016-Mar-30
 */

#include "Fe65p2TriggerLoop.h"

Fe65p2TriggerLoop::Fe65p2TriggerLoop() : LoopActionBase() {
    m_trigCnt = 50; // Maximum numberof triggers to send
    m_trigDelay = 33; // Delay between injection and trigger
    m_trigFreq = 1e3; // 1kHz
    m_trigTime = 10; // 10s
    m_trigWord[0] = 0x00;
    m_trigWord[1] = 0x00;
    m_trigWord[2] = 0x00;
    m_trigWord[3] = MOJO_HEADER + (PULSE_REG << 16) + PULSE_INJECT;
    min = 0;
    max = 0;
    step = 1;
    m_done = false;
    loopType = typeid(this);
}

void Fe65p2TriggerLoop::init() {
    // Setup Trigger
    if (m_trigCnt > 0) {
        g_tx->setTrigConfig(INT_COUNT);
    } else {
        g_tx->setTrigConfig(INT_TIME);
    }
    g_tx->setTrigFreq(m_trigFreq);
    g_tx->setTrigCnt(m_trigCnt);
    g_tx->setTrigWordLength(m_trigWordLength);
    g_tx->setTrigWord(m_trigWord);
    g_tx->setTrigTime(m_trigTime);
    while(!g_tx->isCmdEmpty());
}

void Fe65p2TriggerLoop::end() {

}

void Fe65p2TriggerLoop::execPart1() {
    // Enable Trigger
    g_tx->setTrigEnable(0x1);

}

void Fe65p2TriggerLoop::execPart2() {
    while(!g_tx->isTrigDone()); // We shouldnt get here, cause the inner data loop waits already
    // Disable Trigger
    g_tx->setTrigEnable(0x0);
    m_done = true;
}

void Fe65p2TriggerLoop::setTrigCnt(unsigned int cnt) {
    m_trigCnt = cnt;
}

unsigned int Fe65p2TriggerLoop::getTrigCnt() {
    return m_trigCnt;
}

void Fe65p2TriggerLoop::setTrigFreq(double freq) {
    m_trigFreq = freq;
}

double Fe65p2TriggerLoop::getTrigFreq() {
    return m_trigFreq;
}

void Fe65p2TriggerLoop::setTrigTime(double time) {
    m_trigTime = time;
}

double Fe65p2TriggerLoop::getTrigTime() {
    return m_trigTime;
}
