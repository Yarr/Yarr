// #################################
// # Author: Timon Heim
// # Email: timon.heim at cern.ch
// # Project: Yarr
// # Description: Trigger Loop for RD53A
// # Date: 02/2018
// ################################

#include "Rd53aTriggerLoop.h"

Rd53aTriggerLoop::Rd53aTriggerLoop() : LoopActionBase() {
    m_trigCnt = 50;
    m_trigDelay = 48;
    m_trigFreq = 1e3;
    m_trigTime = 10;
    m_trigWordLength = 16;
    m_trigWord.fill(0x69696969);
    m_trigWord[15] = 0x69696363;
    m_trigWord[14] = Rd53aCmd::genCal(8, 0, 0, 1, 0, 0); // Inject
    m_trigWord[8] = Rd53aCmd::genTrigger(0xF, 4, 0xF, 8); // Trigger
    m_trigWord[7] = Rd53aCmd::genTrigger(0xF, 4, 0xF, 8); // Trigger
    m_trigWord[1] = 0x69696363; // TODO might include ECR?
    m_trigWord[0] = Rd53aCmd::genCal(8, 1, 0, 0, 0, 0); // Arm inject

    min = 0;
    max = 0;
    step = 1;

    isInner = false;
    loopType = typeid(this);
}

void Rd53aTriggerLoop::setTrigDelay(uint32_t delay) {
    if ((delay >= 8) && (delay <= 88)) {
        m_trigDelay = delay;
        m_trigWord[(13-(delay/8)+1)] = Rd53aCmd::genTrigger(0xF, 4, 0xF, 8); // Trigger
        m_trigWord[(13-(delay/8))] = Rd53aCmd::genTrigger(0xF, 4, 0xF, 8); // Trigger
    } else {
        std::cerr << __PRETTY_FUNCTION__ << " : Delay is either too small or too large!" << std::endl;
    }
}

void Rd53aTriggerLoop::init() {
    m_done = false;
    if (verbose)
        std::cout << __PRETTY_FUNCTION__ << std::endl;

    if (m_trigCnt > 0) {
        g_tx->setTrigConfig(INT_COUNT);
    } else {
        g_tx->setTrigConfig(INT_TIME);
    }
    g_tx->setTrigFreq(m_trigFreq);
    g_tx->setTrigCnt(m_trigCnt);
    g_tx->setTrigWord(&m_trigWord[0], 16);
    g_tx->setTrigWordLength(m_trigWordLength);
    g_tx->setTrigTime(m_trigTime);

    g_tx->setCmdEnable(keeper->getTxMask());
    while(!g_tx->isCmdEmpty());
}

void Rd53aTriggerLoop::execPart1() {
    if (verbose)
        std::cout << __PRETTY_FUNCTION__ << std::endl;
    g_tx->setTrigEnable(0x1);

}

void Rd53aTriggerLoop::execPart2() {
    if (verbose)
        std::cout << __PRETTY_FUNCTION__ << std::endl;
    // Should be finished, lets wait anyway
    while(!g_tx->isTrigDone());
    // Disable Trigger
    g_tx->setTrigEnable(0x0);
    m_done = true;
}

void Rd53aTriggerLoop::end() {
    if (verbose)
        std::cout << __PRETTY_FUNCTION__ << std::endl;
    //Nothing to do
}

void Rd53aTriggerLoop::writeConfig(json &config) {
    config["count"] = m_trigCnt;
    config["frequency"] = m_trigFreq;
    config["time"] = m_trigTime;
    config["delay"] = m_trigDelay;
    config["noInject"] = m_noInject;
}

void Rd53aTriggerLoop::loadConfig(json &config) {
    // TODO implement me
}
