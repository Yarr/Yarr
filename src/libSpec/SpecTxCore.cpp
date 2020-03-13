// #################################
// # Author: Timon Heim
// # Email: timon.heim at cern.ch
// # Project: Yarr
// # Description: YARR FW Library
// # Comment: Transmitter Core
// ################################

#include "SpecTxCore.h"

#include "logging.h"

namespace {
    auto stxlog = logging::make_log("SpecTx");
}

SpecTxCore::SpecTxCore() {
    enMask = 0x0;
    m_clk_period = 6.25e-9; // 160MHz base for RD53A
    //m_clk_period = 25.0e-9; // 40MHz base for FE-I4
}

void SpecTxCore::writeFifo(uint32_t value) {
    SPDLOG_LOGGER_TRACE(stxlog, "Value {0:x}", value);
    SpecCom::writeSingle(TX_ADDR | TX_FIFO, value);
}

void SpecTxCore::setCmdEnable(uint32_t value) {
    uint32_t mask = (1 << value);
    SPDLOG_LOGGER_TRACE(stxlog, "Value {0:x}", value);
    SpecCom::writeSingle(TX_ADDR | TX_ENABLE, mask);
    enMask = mask;
}

void SpecTxCore::setCmdEnable(std::vector<uint32_t> channels) {
    uint32_t mask = 0;
    for (uint32_t channel : channels) {
        mask |= (1 << channel);
    }
    SPDLOG_LOGGER_TRACE(stxlog, "Value {0:x}", mask);
    SpecCom::writeSingle(TX_ADDR | TX_ENABLE, mask);
    enMask = mask;
}

void SpecTxCore::disableCmd() {
    SPDLOG_LOGGER_TRACE(stxlog, "");
    SpecCom::writeSingle(TX_ADDR | TX_ENABLE, 0x0);
}

uint32_t SpecTxCore::getCmdEnable() {
    return SpecCom::readSingle(TX_ADDR | TX_ENABLE);
}

void SpecTxCore::maskCmdEnable(uint32_t value, uint32_t mask) {
    uint32_t tmp = SpecCom::readSingle(TX_ADDR | TX_ENABLE);
    tmp &= ~mask;
    value |= tmp;
    SPDLOG_LOGGER_TRACE(stxlog, "Value {0:x}", value);
    SpecCom::writeSingle(TX_ADDR | TX_ENABLE, value);
}

void SpecTxCore::setTrigEnable(uint32_t value) {
    SPDLOG_LOGGER_TRACE(stxlog, "Value {0:x}", value);
    SpecCom::writeSingle(TX_ADDR | TRIG_EN, value);
}

uint32_t SpecTxCore::getTrigEnable() {
    return SpecCom::readSingle(TX_ADDR | TRIG_EN);
}

void SpecTxCore::maskTrigEnable(uint32_t value, uint32_t mask) {
    uint32_t tmp = SpecCom::readSingle(TX_ADDR | TX_ENABLE);
    tmp &= ~mask;
    value |= tmp;
    SPDLOG_LOGGER_TRACE(stxlog, "Value {0:x}", value);
    SpecCom::writeSingle(TX_ADDR | TRIG_EN, value);
}

void SpecTxCore::setTrigConfig(enum TRIG_CONF_VALUE cfg) {
    SPDLOG_LOGGER_TRACE(stxlog, "Config {0:x}", cfg);
    SpecCom::writeSingle(TX_ADDR | TRIG_CONF, (uint32_t) cfg);
}

void SpecTxCore::setTrigFreq(double freq) {
    SPDLOG_LOGGER_TRACE(stxlog, "Frequency {}", freq/1.0e3);
    uint32_t tmp = 1.0/((double)m_clk_period * freq);
    SpecCom::writeSingle(TX_ADDR | TRIG_FREQ, tmp);
}

void SpecTxCore::setTrigCnt(uint32_t count) {
    SPDLOG_LOGGER_TRACE(stxlog, "Count {}", count);
    SpecCom::writeSingle(TX_ADDR | TRIG_COUNT, count);
}

  
void SpecTxCore::setTrigTime(double time) {
    SPDLOG_LOGGER_TRACE(stxlog, "Time {}", time);
    uint64_t tmp = (1.0/(double)m_clk_period)*time;
    SpecCom::writeBlock(TX_ADDR | TRIG_TIME, (uint32_t*)&tmp, 2);
}

void SpecTxCore::setTrigWordLength(uint32_t length) {
    SPDLOG_LOGGER_TRACE(stxlog, "Length {}", length);
    SpecCom::writeSingle(TX_ADDR | TRIG_WORD_LENGTH, length);
}

void SpecTxCore::setTrigWord(uint32_t *word, uint32_t length) {
    for (unsigned i=0; i<length; i++) {
        SPDLOG_LOGGER_TRACE(stxlog, "[{}] = {0:x}", i, word[i]);
    }

    for (unsigned i=0; i<length; i++) {
        SpecCom::writeSingle(TX_ADDR | TRIG_WORD_POINTER, i);
        SpecCom::writeSingle(TX_ADDR | TRIG_WORD, word[i]);
    }
}

void SpecTxCore::toggleTrigAbort() {
    SPDLOG_LOGGER_DEBUG(stxlog, "Toggling trigger abort!");
    SpecCom::writeSingle(TX_ADDR | TRIG_ABORT, 0x1);
}

bool SpecTxCore::isTrigDone() {
    return SpecCom::readSingle(TX_ADDR | TRIG_DONE);
}
 
bool SpecTxCore::isCmdEmpty() {
    std::this_thread::sleep_for(std::chrono::microseconds(10));
    return (SpecCom::readSingle(TX_ADDR | TX_EMPTY) & enMask);
}

uint32_t SpecTxCore::getTrigInCount() {
    return (SpecCom::readSingle(TX_ADDR | TRIG_IN_CNT));
}

void SpecTxCore::setTxPolarity(uint32_t value) {
    SpecCom::writeSingle(TX_ADDR | TX_POLARITY, value);
}

uint32_t SpecTxCore::getTxPolarity() {
    return SpecCom::readSingle(TX_ADDR | TX_POLARITY);
}
