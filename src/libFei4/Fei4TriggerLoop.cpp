/*
 * Authors: T. Heim <timon.heim@cern.ch>,
 * Date: 2013-Oct-22
 */

#include "Fei4TriggerLoop.h"
#include <unistd.h>

Fei4TriggerLoop::Fei4TriggerLoop() : LoopActionBase() {
    m_trigCnt = 50; // Maximum numberof triggers to send
    m_trigDelay = 33; // Delay between injection and trigger
    m_trigFreq = 1e3; // 1kHz
    m_trigTime = 10; // 10s
    m_trigWord[0] = 0x00;
    m_trigWord[1] = TRIG_CMD;
    m_trigWord[2] = 0x00;
    m_trigWord[3] = CAL_CMD;
    m_trigWordLength = 4;
    m_noInject = false;
    m_extTrigger = false;
    isInner = false;
    min = 0;
    max = 0;
    step = 1;
    loopType = typeid(this);
}

void Fei4TriggerLoop::init() {
    m_done = false;
    if (verbose)
        std::cout << __PRETTY_FUNCTION__ << std::endl;
    // Setup Trigger
    this->setTrigDelay(m_trigDelay);
    if (m_trigCnt > 0) {
        g_tx->setTrigConfig(INT_COUNT);
    } else {
        g_tx->setTrigConfig(INT_TIME);
    }
    g_tx->setTrigFreq(m_trigFreq);
    g_tx->setTrigCnt(m_trigCnt);
    g_tx->setTrigWordLength(m_trigWordLength);
    g_tx->setTrigWord(m_trigWord, m_trigWordLength);
    g_tx->setTrigTime(m_trigTime);
    // Set active Modules into runmode
    // TODO ISSUE: This makes problems
    /*for (unsigned i=0; i<keeper->feList.size(); i++) {
        if (keeper->feList[i]->isActive()) {
            std::cout << "Tx = " << (0x1 << keeper->feList[i]->getTxChannel()) << std::endl;
            g_tx->setCmdEnable(0x1 << keeper->feList[i]->getTxChannel());
            keeper->feList[i]->setRunMode(true);
            usleep(100);
            while(!g_tx->isCmdEmpty());
        }
    }*/

    // Workaround: Put everything into run mode, active rx channels will sort this out
    g_tx->setCmdEnable(keeper->getTxMask());
    keeper->globalFe<Fei4>()->setRunMode(true);
    usleep(100); // Empty could be delayed
    while(!g_tx->isCmdEmpty());
}

void Fei4TriggerLoop::end() {
    if (verbose)
        std::cout << __PRETTY_FUNCTION__ << std::endl;
    // Go back to conf mode, general state of FE should be conf mode
    keeper->globalFe<Fei4>()->setRunMode(false);
    while(!g_tx->isCmdEmpty());
}

void Fei4TriggerLoop::execPart1() {
    if (verbose)
        std::cout << __PRETTY_FUNCTION__ << std::endl;
    // Enable Trigger
    g_tx->setTrigEnable(0x1);
}

void Fei4TriggerLoop::execPart2() {
    if (verbose)
        std::cout << __PRETTY_FUNCTION__ << std::endl;
    while(!g_tx->isTrigDone());
    // Disable Trigger
    g_tx->setTrigEnable(0x0);
    m_done = true;
}

void Fei4TriggerLoop::setTrigCnt(unsigned int cnt) {
    m_trigCnt = cnt;
}

unsigned int Fei4TriggerLoop::getTrigCnt() {
    return m_trigCnt;
}

void Fei4TriggerLoop::setTrigDelay(unsigned int delay) {
    unsigned pos = (delay-1)%32; // subtract 8 bit long trig cmd
    unsigned word = (delay-1)/32; // Select word in array
    m_trigWord[0] = 0;
    m_trigWord[1] = 0;
    m_trigWord[2] = 0;
    m_trigWord[3] = CAL_CMD;
    if ((word < 3 && pos <= 27) || word < 2) {
        m_trigWord[2-word] = (TRIG_CMD>>pos);
        if (pos > 27) // In case we shifted over word border
            m_trigWord[2-1-word] = (TRIG_CMD<<(5-(32-pos)));
        m_trigDelay = delay;
    }
    m_trigWordLength = 4;
}

void Fei4TriggerLoop::setNoInject() {
    m_trigWord[0] = 0;
    m_trigWord[1] = 0;
    m_trigWord[2] = 0;
    m_trigWord[3] = TRIG_CMD;
    m_trigWordLength = 4;
}

void Fei4TriggerLoop::setNoWord() {
    m_trigWord[0] = 0;
    m_trigWord[1] = 0;
    m_trigWord[2] = 0;
    m_trigWord[3] = 0;
}

unsigned int Fei4TriggerLoop::getTrigDelay() {
    return m_trigDelay;
}

void Fei4TriggerLoop::setTrigFreq(double freq) {
    m_trigFreq = freq;
}

double Fei4TriggerLoop::getTrigFreq() {
    return m_trigFreq;
}

void Fei4TriggerLoop::setTrigTime(double time) {
    m_trigTime = time;
}

double Fei4TriggerLoop::getTrigTime() {
    return m_trigTime;
}

/*
void Fei4TriggerLoop::setIsInner(bool itis) {
    isInner = itis;
}

bool Fei4TriggerLoop::getIsInner() {
    return isInner;
}
*/

void Fei4TriggerLoop::writeConfig(json &config) {
    config["count"] = m_trigCnt;
    config["frequency"] = m_trigFreq;
    config["time"] = m_trigTime;
    config["delay"] = m_trigDelay;
    config["noInject"] = m_noInject;
    config["extTrigger"] = m_extTrigger;
}

void Fei4TriggerLoop::loadConfig(json &config) {
    m_trigCnt = config["count"];
    m_trigFreq = config["frequency"];
    m_trigTime = config["time"];
    m_trigDelay = config["delay"];
    // TODO these two don't do anything yet
    m_noInject = config["noInject"];
    m_extTrigger = config["extTrigger"];
}


void Fei4TriggerLoop::setTrigWord(uint32_t word[4]) {
    m_trigWord[0] = word[0];
    m_trigWord[1] = word[1];
    m_trigWord[2] = word[2];
    m_trigWord[3] = word[3];
}
