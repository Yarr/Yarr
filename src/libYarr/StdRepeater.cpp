/*
 * Authors: T. Heim <timon.heim@cern.ch>,
 * Date: 2018-Aug
 */

#include "StdRepeater.h"

#include <iostream>

#include "logging.h"

namespace {
    auto srlog = logging::make_log("StdRepeater");
}

// Query style?
StdRepeater::StdRepeater() : LoopActionBase(LOOP_STYLE_MASK) {
    loopType = typeid(this);
    min = 0;
    max = 0;
    step = 1;
    m_cur = 0;
    m_done = false;
}

void StdRepeater::init() {
    m_done = false;
    SPDLOG_LOGGER_TRACE(srlog, "");
    m_cur = min;
}

void StdRepeater::end() {
    SPDLOG_LOGGER_TRACE(srlog, "");
}

void StdRepeater::execPart1() {
    SPDLOG_LOGGER_TRACE(srlog, "");
    m_cur++;
}

void StdRepeater::execPart2() {
    SPDLOG_LOGGER_TRACE(srlog, "");
    if (m_cur == max)
        m_done = true;
}

void StdRepeater::writeConfig(json &j) {
    j["min"] = min;
    j["max"] = max;
}

void StdRepeater::loadConfig(json &j) {
    if (j.contains("min"))
        min = j["min"];
    if (j.contains("max"))
        max = j["max"];
}
