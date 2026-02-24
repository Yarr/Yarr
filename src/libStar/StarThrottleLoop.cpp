/*
 */

#include "StarThrottleLoop.h"

#include "Bookkeeper.h"
#include "logging.h"

namespace {
    auto logger = logging::make_log("StarThrottleLoop.Experimental");
}


StarThrottleLoop::StarThrottleLoop() : LoopActionBase(LOOP_STYLE_GLOBAL_FEEDBACK) {
    min = 0;
    max = 10000;
    step = 128;
    loopType = typeid(this);
    m_done = false;
}

StarThrottleLoop::StarThrottleLoop(Register StarCfg::*ref) : LoopActionBase(LOOP_STYLE_GLOBAL_FEEDBACK), parPtr(ref) {
    min = 0;
    max = 10000;
    step = 128;
    loopType = typeid(this);
    m_done = false;
}

void StarThrottleLoop::writeConfig(json &j) {
    j["max"] = max;
    j["step"] = step;
    j["max_iters"] = max_iters;
}

void StarThrottleLoop::loadConfig(const json &j) {
    if (!j["max"].empty())
        max = j["max"];
    if (!j["step"].empty())
        step = j["step"];
    if (!j["max_iters"].empty())
        max_iters = j["max_iters"];
}

void StarThrottleLoop::feedback(unsigned channel, double sign, bool last) {
    m_done = last;
}

void StarThrottleLoop::init() {
    m_done = false;
}

void StarThrottleLoop::execPart1() {
}

void StarThrottleLoop::execPart2() {
    for(unsigned id=0; id<keeper->getNumOfEntries(); id++) {
        auto &fe = *keeper->getEntry(id).fe;
        if (fe.getActive()) {
            unsigned rx = dynamic_cast<FrontEndCfg&>(fe).getRxChannel();
            waitForFeedback(rx);
        }
    }
}

void StarThrottleLoop::end() {
}
