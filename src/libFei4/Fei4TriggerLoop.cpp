/*
 * Authors: T. Heim <timon.heim@cern.ch>,
 * Date: 2013-Oct-22
 */

#include "Fei4TriggerLoop.h"
#include <unistd.h>

#include "logging.h"

namespace {
    auto logger = logging::make_log("Fei4TriggerLoop");
}

Fei4TriggerLoop::Fei4TriggerLoop() : LoopActionBase(LOOP_STYLE_TRIGGER) {
    setTrigCnt(50); // Maximum numberof triggers to send
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
    SPDLOG_LOGGER_TRACE(logger, "");
    m_done = false;
    // Setup Trigger
    this->setTrigDelay(m_trigDelay);
    if (getTrigCnt() > 0) {
        g_tx->setTrigConfig(INT_COUNT);
    } else {
        g_tx->setTrigConfig(INT_TIME);
    }
    g_tx->setTrigFreq(m_trigFreq);
    g_tx->setTrigCnt(getTrigCnt());
    g_tx->setTrigWordLength(m_trigWordLength);
    g_tx->setTrigWord(m_trigWord, m_trigWordLength);
    g_tx->setTrigTime(m_trigTime);
    // Set active Modules into runmode

    // Workaround: Put everything into run mode, active rx channels will sort this out
    g_tx->setCmdEnable(keeper->getTxMask());
    keeper->globalFe<Fei4>()->setRunMode(true);
    usleep(100); // Empty could be delayed
    while(!g_tx->isCmdEmpty());
}

void Fei4TriggerLoop::end() {
    SPDLOG_LOGGER_TRACE(logger, "");
    // Go back to conf mode, general state of FE should be conf mode
    keeper->globalFe<Fei4>()->setRunMode(false);
    while(!g_tx->isCmdEmpty());
}

void Fei4TriggerLoop::execPart1() {
    SPDLOG_LOGGER_TRACE(logger, "");
    // Enable Trigger
    g_tx->setTrigEnable(0x1);
}

void Fei4TriggerLoop::execPart2() {
    SPDLOG_LOGGER_TRACE(logger, "");
    while(!g_tx->isTrigDone());
    // Disable Trigger
    g_tx->setTrigEnable(0x0);
    m_done = true;
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

unsigned int Fei4TriggerLoop::getTrigDelay() const {
    return m_trigDelay;
}

void Fei4TriggerLoop::setTrigFreq(double freq) {
    m_trigFreq = freq;
}

double Fei4TriggerLoop::getTrigFreq() const {
    return m_trigFreq;
}

void Fei4TriggerLoop::setTrigTime(double time) {
    m_trigTime = time;
}

double Fei4TriggerLoop::getTrigTime() const {
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
    config["count"] = getTrigCnt();
    config["frequency"] = (float) m_trigFreq; //variant fix
    config["time"] = m_trigTime;
    config["delay"] = m_trigDelay;
    config["noInject"] = m_noInject;
    config["extTrigger"] = m_extTrigger;
}

void Fei4TriggerLoop::loadConfig(const json &config) {
    if (config.contains("count"))
      setTrigCnt(config["count"]);
    if (config.contains("frequency"))
      m_trigFreq = config["frequency"];
    if (config.contains("time"))
      m_trigTime = config["time"];
    if (config.contains("delay"))
      m_trigDelay = config["delay"];
    // TODO these two don't do anything yet
    if (config.contains("noInject"))
      m_noInject = config["noInject"];
    if (config.contains("extTrigger"))
      m_extTrigger = config["extTrigger"];
}


void Fei4TriggerLoop::setTrigWord(uint32_t word[4]) {
    m_trigWord[0] = word[0];
    m_trigWord[1] = word[1];
    m_trigWord[2] = word[2];
    m_trigWord[3] = word[3];
}

uint32_t Fei4TriggerLoop::getExpEvents(){
    return getTrigCnt()*keeper->globalFe<Fei4>()->readRegister(&Fei4::Trig_Count);
}
