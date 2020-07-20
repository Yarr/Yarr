// #################################
// # Author: Timon Heim
// # Email: timon.heim at cern.ch
// # Project: Yarr
// # Description: Trigger Loop for RD53B
// # Date: 07/2020
// ################################

#include "Rd53bTriggerLoop.h"

#include "logging.h"

namespace {
    auto logger = logging::make_log("Rd53bTriggerLoop");
}

Rd53bTriggerLoop::Rd53bTriggerLoop() : LoopActionBase() {
    m_trigCnt = 50;
    m_trigDelay = 48;
    m_trigFreq = 1e3;
    m_trigTime = 10;
    m_trigWordLength = 32;
    m_pulseDuration = 8;
    m_trigWord.fill(0xAAAAAAAA);
    m_noInject = false;
    m_extTrig = false;
    m_trigMultiplier = 16;

    m_edgeMode = false;
    m_edgeDuration = 40;

    min = 0;
    max = 0;
    step = 1;

    loopType = typeid(this);
}

void Rd53bTriggerLoop::setTrigDelay(uint32_t delay) {
    m_trigWord.fill(0xAAAAAAAA);
    
    // Injection command
    std::array<uint16_t, 3> calWords = Rd53b::genCal(16, 0, 0, 1, 0, 0);
    m_trigWord[31] = 0xAAAA0000 | calWords[0];
    m_trigWord[30] = ((uint32_t)calWords[1]<<16) | calWords[2];
    
    uint64_t trigStream = 0;

    // Generate stream of ones for each trigger
    uint64_t one = 1;
    for (unsigned i=0; i<m_trigMultiplier; i++)
        trigStream |= (one << i);
    trigStream = trigStream << delay%8;

    for (unsigned i=0; i<(m_trigMultiplier/8)+1; i++) {
        if (((30-(delay/8)-i) > 2) && delay > 30) {
            uint32_t bc1 = (trigStream >> (2*i*4)) & 0xF;
            uint32_t bc2 = (trigStream >> ((2*i*4)+4)) & 0xF;
            m_trigWord[30-(delay/8)-i] = ((uint32_t)Rd53b::genTrigger(bc1, 2*i)[0] << 16) |  Rd53b::genTrigger(bc2, (2*i)+1)[0];
        } else {
            logger->error("Delay is either too small or too large!");
        }
    }

    // Rearm
    std::array<uint16_t, 3> armWords = Rd53b::genCal(16, 1, 0, 0, 0, 0);
    m_trigWord[1] = 0xAAAA0000 | armWords[0];
    m_trigWord[0] = ((uint32_t)armWords[1]<<16) | armWords[2];
    
    logger->debug("Trigger buffer set to:");
    for (unsigned i=0; i<m_trigWordLength; i++) {
      logger->debug("[{}: 0x{:x}", 31-i, m_trigWord[31-i]);
    }
}

void Rd53bTriggerLoop::setEdgeMode(uint32_t duration) {
    // Assumes CAL command to be in index 31/30
    std::array<uint16_t, 3> calWords = Rd53b::genCal(16, 1, 0, 20, 0, 0);
    m_trigWord[31] = 0xAAAA0000 | calWords[0];
    m_trigWord[30] = ((uint32_t)calWords[1]<<16) | calWords[2];
}

void Rd53bTriggerLoop::setNoInject() {
    m_trigWord[31] = 0xAAAAAAAA;
    m_trigWord[30] = 0xAAAAAAAA;
    m_trigWord[1] = 0xAAAAAAAA;
    m_trigWord[0] = 0xAAAAAAAA;

}

void Rd53bTriggerLoop::init() {
    SPDLOG_LOGGER_TRACE(logger, "");
    m_done = false;

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

void Rd53bTriggerLoop::execPart1() {
    SPDLOG_LOGGER_TRACE(logger, "");
    g_tx->setCmdEnable(keeper->getTxMask());
    auto rd53b = dynamic_cast<Rd53b*>(g_fe);
    //rd53b->sendClear(16);
    while(!g_tx->isCmdEmpty());
    //std::this_thread::sleep_for(std::chrono::microseconds(200));
    g_rx->flushBuffer();
    std::this_thread::sleep_for(std::chrono::microseconds(10));
    //std::this_thread::sleep_for(std::chrono::milliseconds(10));
    g_tx->setTrigEnable(0x1);

}

void Rd53bTriggerLoop::execPart2() {
    SPDLOG_LOGGER_TRACE(logger, "");
    // Should be finished, lets wait anyway
    while(!g_tx->isTrigDone());
    // Disable Trigger
    g_tx->setTrigEnable(0x0);
    m_done = true;
}

void Rd53bTriggerLoop::end() {
    SPDLOG_LOGGER_TRACE(logger, "");
    //Nothing to do
}

void Rd53bTriggerLoop::writeConfig(json &config) {
    config["count"] = m_trigCnt;
    config["frequency"] = m_trigFreq;
    config["time"] = m_trigTime;
    config["delay"] = m_trigDelay;
    config["noInject"] = m_noInject;
    config["extTrig"] = m_extTrig;
    config["trigMultiplier"] = m_trigMultiplier;
}

void Rd53bTriggerLoop::loadConfig(json &config) {
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
