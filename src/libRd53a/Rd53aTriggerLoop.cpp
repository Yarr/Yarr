// #################################
// # Author: Timon Heim
// # Email: timon.heim at cern.ch
// # Project: Yarr
// # Description: Trigger Loop for RD53A
// # Date: 02/2018
// ################################

#include "Rd53aTriggerLoop.h"

#include "logging.h"

namespace {
  auto logger = logging::make_log("Rd53aTriggerLoop");
}

Rd53aTriggerLoop::Rd53aTriggerLoop() : LoopActionBase(LOOP_STYLE_TRIGGER) {
    setTrigCnt(50);
    m_trigDelay = 48;
    m_trigFreq = 1e3;
    m_trigTime = 10;
    m_trigWordLength = 32;
    m_pulseDuration = 8;
    m_trigWord.fill(0x69696969);
    m_noInject = false;
    m_extTrig = false;
    m_trigMultiplier = 16;
    m_sendEcr = false;

    m_edgeMode = false;
    m_edgeDuration = 10;

    min = 0;
    max = 0;
    step = 1;
    progress = 0;

    isInner = false;
    loopType = typeid(this);
}

void Rd53aTriggerLoop::setTrigDelay(uint32_t delay) {
    m_trigWord.fill(0x69696969);
    // Inject

    // ECR for sync FE
    if (m_sendEcr) {
        m_trigWord[16] = 0x5a5a6969;
    }
    
    //m_trigWord[30] = 0x59596969;
    m_trigWord[15] = 0x69696363; // Header
    m_trigWord[14] = Rd53aCmd::genCal(8, 0, 0, 1, 0, 0); // Inject
    uint64_t trigStream = 0;

    // Generate stream of ones for each trigger
    uint64_t one = 1;
    for (unsigned i=0; i<m_trigMultiplier; i++)
        trigStream |= (one << i);
    trigStream = trigStream << delay%8;

    for (unsigned i=0; i<(m_trigMultiplier/8)+1; i++) {
        if (((14-(delay/8)-i) > 2) && delay > 16) {
            uint32_t bc1 = (trigStream >> (2*i*4)) & 0xF;
            uint32_t bc2 = (trigStream >> ((2*i*4)+4)) & 0xF;
            m_trigWord[14-(delay/8)-i] = Rd53aCmd::genTrigger(bc1, 2*i, bc2, (2*i)+1);
        } else {
            SPDLOG_LOGGER_ERROR(logger, "Delay is either too small or too large!");
        }
    }
    // Rearm
    m_trigWord[2] = 0x69696363; // Header
    m_trigWord[1] = Rd53aCmd::genCal(8, 1, 0, 0, 0, 0); // Arm inject
    // Global Pulse (software AZ) for sync FE
    if (g_tx->getSoftwareAZ()) {
        m_trigWord[0] = 0x5c5c0000 + (Rd53aCmd::encode5to8(0x8<<1)<<8) + (Rd53aCmd::encode5to8(m_pulseDuration<<1));
    }
    logger->debug("Trigger buffer set to:");
    for (unsigned i=0; i<m_trigWordLength; i++) {
      logger->debug("[{}: 0x{:x}", 31-i, m_trigWord[31-i]);
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
    SPDLOG_LOGGER_TRACE(logger, "");
    m_done = false;

    this->setTrigDelay(m_trigDelay);
    if (m_edgeMode)
        this->setEdgeMode(m_edgeDuration);
    if (m_extTrig) {
        g_tx->setTrigConfig(EXT_TRIGGER);
    } else if (getTrigCnt() > 0) {
        g_tx->setTrigConfig(INT_COUNT);
    } else {
        g_tx->setTrigConfig(INT_TIME);
    }
    if (m_noInject) {
        setNoInject();
    }
    g_tx->setTrigFreq(m_trigFreq);
    g_tx->setTrigCnt(getTrigCnt());
    g_tx->setTrigWord(&m_trigWord[0], 32);
    g_tx->setTrigWordLength(m_trigWordLength);
    g_tx->setTrigTime(m_trigTime);

    g_tx->setCmdEnable(keeper->getTxMask());
    while(!g_tx->isCmdEmpty());
    //std::this_thread::sleep_for(std::chrono::milliseconds(1));
}

void Rd53aTriggerLoop::execPart1() {
    SPDLOG_LOGGER_TRACE(logger, "");
    g_tx->setCmdEnable(keeper->getTxMask());
    dynamic_cast<HwController*>(g_tx)->runMode();
    auto rd53a = dynamic_cast<Rd53a*>(g_fe);
    rd53a->ecr();
    rd53a->idle();
    rd53a->idle();
    rd53a->idle();
    rd53a->idle();
    // AZ level for sync FE 
    if (g_tx->getSoftwareAZ()) {
        rd53a->globalPulse(8, m_pulseDuration);
    }
    std::this_thread::sleep_for(std::chrono::microseconds(200));
    g_rx->flushBuffer();
    while(!g_tx->isCmdEmpty());
    std::this_thread::sleep_for(std::chrono::microseconds(10));
    g_tx->setTrigEnable(0x1);

}

void Rd53aTriggerLoop::execPart2() {
    SPDLOG_LOGGER_TRACE(logger, "");
    // Should be finished, lets wait anyway
    while(!g_tx->isTrigDone());
    // Disable Trigger
    g_tx->setTrigEnable(0x0);
    dynamic_cast<HwController*>(g_tx)->setupMode();
    m_done = true;
    progress = 1;
}

void Rd53aTriggerLoop::end() {
    SPDLOG_LOGGER_TRACE(logger, "");
    //Nothing to do
}

void Rd53aTriggerLoop::writeConfig(json &config) {
    config["count"] = getTrigCnt();
    config["frequency"] = m_trigFreq;
    config["time"] = m_trigTime;
    config["delay"] = m_trigDelay;
    config["noInject"] = m_noInject;
    config["extTrig"] = m_extTrig;
    config["trigMultiplier"] = m_trigMultiplier;
    config["sendEcr"] = m_sendEcr;
}

void Rd53aTriggerLoop::loadConfig(const json &config) {
    if (config.contains("count"))
        setTrigCnt(config["count"]);
    if (config.contains("frequency"))
        m_trigFreq = config["frequency"];
    if (config.contains("time"))
        m_trigTime = config["time"];
    if (config.contains("delay"))
        m_trigDelay = config["delay"];
    if (config.contains("noInject"))
        m_noInject = config["noInject"];
    if (config.contains("edgeMode"))
        m_edgeMode = config["edgeMode"];
    if (config.contains("extTrig"))
        m_extTrig = config["extTrig"];
    if (config.contains("trigMultiplier"))
        m_trigMultiplier = config["trigMultiplier"];
    if (config.contains("sendEcr"))
        m_sendEcr = config["sendEcr"];
}

uint32_t Rd53aTriggerLoop::getExpEvents(){
    return getTrigCnt()*m_trigMultiplier;
}
