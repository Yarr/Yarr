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

Rd53bTriggerLoop::Rd53bTriggerLoop() : LoopActionBase(LOOP_STYLE_TRIGGER) {
    setTrigCnt(50);
    m_trigDelay = 48;
    m_calEdgeDelay = 0;
    m_trigFreq = 1e3;
    m_trigTime = 10;
    m_trigWordLength = 32;
    m_pulseDuration = 8;
    m_trigWord.fill(0xAAAAAAAA);
    m_noInject = false;
    m_extTrig = false;
    m_trigMultiplier = 16;
    m_zeroTot = true;

    m_edgeMode = false;
    m_edgeDuration = 40;

    min = 0;
    max = 0;
    step = 1;

    loopType = typeid(this);
}

void Rd53bTriggerLoop::setTrigDelay(uint32_t delay, uint32_t cal_edge_delay=0) {
    m_trigWord.fill(0xAAAAAAAA);
    
    // Injection command
    std::array<uint16_t, 3> calWords = Rd53b::genCal(16, 0, cal_edge_delay, 1, 0, 0);
    m_trigWord[31] = 0xAAAA0000 | calWords[0];
    m_trigWord[30] = ((uint32_t)calWords[1]<<16) | calWords[2];
    
    //std::array<uint16_t, 4> wrReg = Rd53bCmd::genWrReg(16, 53, 0x0);
    //m_trigWord[29] = (((uint32_t)wrReg[0] << 16) | wrReg[1]);
    //m_trigWord[28] = (((uint32_t)wrReg[2] << 16) | wrReg[3]);
    
	// Special case: if trigger multiplier = 0, no trigger should be sent in command line
    if(m_trigMultiplier != 0){
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
    }

    //std::array<uint16_t, 4> wrReg2 = Rd53bCmd::genWrReg(16, 53, 0x80);
    //m_trigWord[3] = (((uint32_t)wrReg2[0] << 16) | wrReg2[1]);
    //m_trigWord[2] = (((uint32_t)wrReg2[2] << 16) | wrReg2[3]);
    
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
    std::array<uint16_t, 3> calWords = Rd53b::genCal(16, 1, 0, duration, 0, 0);
    m_trigWord[31] = 0xAAAA0000 | calWords[0];
    m_trigWord[30] = ((uint32_t)calWords[1]<<16) | calWords[2];
    m_trigWord[1] = 0xAAAAAAAA;
    m_trigWord[0] = 0xAAAAAAAA;
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

    this->setTrigDelay(m_trigDelay, m_calEdgeDelay);
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

void Rd53bTriggerLoop::execPart1() {
    SPDLOG_LOGGER_TRACE(logger, "");
    g_tx->setCmdEnable(keeper->getTxMask());
    auto rd53b = dynamic_cast<Rd53b*>(g_fe);
    dynamic_cast<HwController*>(g_tx)->runMode();
    //rd53b->sendClear(16);
    while(!g_tx->isCmdEmpty());
    std::this_thread::sleep_for(std::chrono::microseconds(200));
    //std::this_thread::sleep_for(std::chrono::microseconds(10));
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
    dynamic_cast<HwController*>(g_tx)->setupMode();
    
    if (m_zeroTot) {
        // Reset ToT memories
        Rd53b *rd53b = dynamic_cast<Rd53b*>(g_fe);
        uint16_t latency = rd53b->Latency.read();
        uint16_t injDigEn = rd53b->InjDigEn.read();
        rd53b->writeRegister(&Rd53b::Latency, 500);
        rd53b->writeRegister(&Rd53b::InjDigEn, 1);

        while(!g_tx->isCmdEmpty()){;}

        this->setEdgeMode(2);
        g_tx->setTrigFreq(800000);
        g_tx->setTrigCnt(100);
        g_tx->setTrigWord(&m_trigWord[0], 32);
        g_tx->setTrigWordLength(32);
        g_tx->setTrigConfig(INT_COUNT);

        g_tx->setTrigEnable(0x1);
        std::this_thread::sleep_for(std::chrono::microseconds(10));
        while(!g_tx->isTrigDone());
        g_tx->setTrigEnable(0x0);

        rd53b->writeRegister(&Rd53b::Latency, latency);
        rd53b->writeRegister(&Rd53b::InjDigEn, injDigEn);
        
        while(!g_tx->isCmdEmpty()){;}
    }
    
    m_done = true;
}

void Rd53bTriggerLoop::end() {
    SPDLOG_LOGGER_TRACE(logger, "");
    //Nothing to do
}

void Rd53bTriggerLoop::writeConfig(json &config) {
    config["count"] = getTrigCnt();
    config["frequency"] = m_trigFreq;
    config["time"] = m_trigTime;
    config["delay"] = m_trigDelay;
    config["calEdgeDelay"] = m_calEdgeDelay;
    config["noInject"] = m_noInject;
    config["extTrig"] = m_extTrig;
    config["trigMultiplier"] = m_trigMultiplier;
    config["edgeMode"] = m_edgeMode;
    config["edgeDuration"] = m_edgeDuration;
    config["zeroTot"] = m_zeroTot;
}

void Rd53bTriggerLoop::loadConfig(const json &config) {
    if (config.contains("count"))
        setTrigCnt(config["count"]);
    if (config.contains("frequency"))
        m_trigFreq = config["frequency"];
    if (config.contains("time"))
        m_trigTime = config["time"];
    if (config.contains("delay"))
        m_trigDelay = config["delay"];
    if (config.contains("calEdgeDelay"))
        m_calEdgeDelay = config["calEdgeDelay"];
    if (config.contains("noInject"))
        m_noInject = config["noInject"];
    if (config.contains("edgeMode"))
        m_edgeMode = config["edgeMode"];
    if (config.contains("edgeDuration"))
        m_edgeDuration = config["edgeDuration"];
    if (config.contains("extTrig"))
        m_extTrig = config["extTrig"];
    if (config.contains("trigMultiplier"))
        m_trigMultiplier = config["trigMultiplier"];
    if (config.contains("zeroTot"))
        m_zeroTot = config["zeroTot"];
    this->setTrigDelay(m_trigDelay, m_calEdgeDelay);
}

uint32_t Rd53bTriggerLoop::getExpEvents(){

    return getTrigCnt()*m_trigMultiplier;
}
