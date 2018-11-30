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
    m_trigWordLength = 32;
    m_pulseDuration = 8;
    m_trigWord.fill(0x69696969);
    m_noInject = false;
    m_extTrig = false;
    m_trigMultiplier = 16;

    m_edgeMode = false;
    m_edgeDuration = 10;

    min = 0;
    max = 0;
    step = 1;

    isInner = false;
    loopType = typeid(this);
    verbose = false;
}

void Rd53aTriggerLoop::setTrigDelay(uint32_t delay) {
    m_trigWord.fill(0x69696969);
    // Inject
    //m_trigWord[31] = 0x5a5a6969;
    m_trigWord[16] = 0x59596969;
    m_trigWord[15] = 0x69696363; // Header
    m_trigWord[14] = Rd53aCmd::genCal(8, 0, 0, 1, 0, 0); // Inject
    uint64_t trigStream = 0;

    // Generate stream of ones for each trigger
    for (unsigned i=0; i<m_trigMultiplier; i++)
        trigStream |= 1 << i;
    trigStream = trigStream << delay%8;

    for (unsigned i=0; i<(m_trigMultiplier/8)+1; i++) {
        if (((14-(delay/8)-i) > 2) && delay > 16) {
            uint32_t bc1 = (trigStream >> (2*i*4)) & 0xF;
            uint32_t bc2 = (trigStream >> ((2*i*4)+4)) & 0xF;
            m_trigWord[14-(delay/8)-i] = Rd53aCmd::genTrigger(bc1, 2*i, bc2, (2*i)+1);
        } else {
            std::cerr << __PRETTY_FUNCTION__ << " : Delay is either too small or too large!" << std::endl;
        }
    }
    // Rearm
    m_trigWord[2] = 0x69696363; // Header
    m_trigWord[1] = Rd53aCmd::genCal(8, 1, 0, 0, 0, 0); // Arm inject
    // Pulse
    //m_trigWord[0] = 0x5c5c0000 + (Rd53aCmd::encode5to8(0x8<<1)<<8) + (Rd53aCmd::encode5to8(m_pulseDuration<<1)); // global pulse for sync FE
    
    if (verbose) {
        std::cout << "Trigger buffer set to:" << std::endl;
        for (unsigned i=0; i<m_trigWordLength; i++) {
            std::cout << "[" << 31-i << "] : 0x" << std::hex << m_trigWord[31-i] << std::dec << std::endl;
        }
    }
}

void Rd53aTriggerLoop::setEdgeMode(uint32_t duration) {
    // Assumes CAL command to be in index 14
    m_trigWord[14] = Rd53aCmd::genCal(8, 1, 0, 40, 0, 0); // Inject
}

void Rd53aTriggerLoop::setNoInject() {
    m_trigWord[15] = 0x69696969;
    m_trigWord[14] = 0x69696969;
    m_trigWord[2] = 0x69696969;
    m_trigWord[1] = 0x69696969;
    //m_trigWord[0] = 0x5c5c0000 + (Rd53aCmd::encode5to8(0x8<<1)<<8) + (Rd53aCmd::encode5to8(m_pulseDuration<<1)); // global pulse for sync FE

}

void Rd53aTriggerLoop::init() {
    m_done = false;
    if (verbose)
        std::cout << __PRETTY_FUNCTION__ << std::endl;

    this->setTrigDelay(m_trigDelay);
    if (m_edgeMode)
        this->setEdgeMode(m_edgeDuration);
    if (m_extTrig) {
        g_tx->setTrigConfig(EXT_TRIGGER);
    } else if (m_trigCnt > 0) {
        g_tx->setTrigConfig(INT_COUNT);
    } else {
        g_tx->setTrigConfig(INT_TIME);
    }
    if (m_noInject) {
        setNoInject();
    }
    g_tx->setTrigFreq(m_trigFreq);
    g_tx->setTrigCnt(m_trigCnt);
    g_tx->setTrigWord(&m_trigWord[0], 32);
    g_tx->setTrigWordLength(m_trigWordLength);
    g_tx->setTrigTime(m_trigTime);

    g_tx->setCmdEnable(keeper->getTxMask());
    while(!g_tx->isCmdEmpty());
    //std::this_thread::sleep_for(std::chrono::milliseconds(1));
}

void Rd53aTriggerLoop::execPart1() {
    if (verbose)
        std::cout << __PRETTY_FUNCTION__ << std::endl;
    dynamic_cast<Rd53a*>(g_fe)->ecr();
    dynamic_cast<Rd53a*>(g_fe)->idle();
    dynamic_cast<Rd53a*>(g_fe)->idle();
    dynamic_cast<Rd53a*>(g_fe)->idle();
    dynamic_cast<Rd53a*>(g_fe)->idle();
    std::this_thread::sleep_for(std::chrono::microseconds(200));
    g_rx->flushBuffer();
    while(!g_tx->isCmdEmpty());
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
    config["extTrigger"] = m_extTrig;
    config["trigMultiplier"] = m_trigMultiplier;
}

void Rd53aTriggerLoop::loadConfig(json &config) {
    if (!config["count"].empty())
        m_trigCnt = config["count"];
    if (!config["frequency"].empty())
        m_trigFreq = config["frequency"];
    if (!config["time"].empty())
        m_trigTime = config["time"];
    if (!config["delay"].empty())
        m_trigDelay = config["delay"];
    if (!config["noInject"].empty())
        m_noInject = config["noInject"];
    if (!config["edgeMode"].empty())
        m_edgeMode = config["edgeMode"];
    if (!config["extTrig"].empty())
        m_extTrig = config["extTrig"];
    if (!config["trigMultiplier"].empty())
        m_trigMultiplier = config["trigMultiplier"];
    this->setTrigDelay(m_trigDelay);
}
