/*
 * Authors: T. Heim <timon.heim@cern.ch>,
 * Date: 2018-Aug
 */

#include "StdRepeater.h"

StdRepeater::StdRepeater() : LoopActionBase() {
    loopType = typeid(this);
    min = 0;
    max = 0;
    step = 1;
    m_cur = 0;
    m_done = false;
}

void StdRepeater::init() {
    m_done = false;
    if (verbose)
        std::cout << __PRETTY_FUNCTION__ << std::endl;
    m_cur = min;
}

void StdRepeater::end() {
    if (verbose)
        std::cout << __PRETTY_FUNCTION__ << std::endl;
}

void StdRepeater::execPart1() {
    if (verbose)
        std::cout << __PRETTY_FUNCTION__ << std::endl;
    m_cur++;
}

void StdRepeater::execPart2() {
    if (verbose)
        std::cout << __PRETTY_FUNCTION__ << std::endl;
    if (m_cur == max)
        m_done = true;
}

void StdRepeater::writeConfig(json &j) {
    j["min"] = min;
    j["max"] = max;
}

void StdRepeater::loadConfig(json &j) {
    if (!j["min"].empty())
        min = j["min"];
    if (!j["max"].empty())
        max = j["max"];
}
