/*
 */

#include "StarThrottleLoop.h"
#include "logging.h"

namespace {
    auto logger = logging::make_log("StarThrottleLoop");
}


StarThrottleLoop::StarThrottleLoop() : LoopActionBase(LOOP_STYLE_GLOBAL_FEEDBACK) {
    min = 0;
    max = 10000;
    step = 1;
    loopType = typeid(this);
    m_done = false;
    max_iters = 100;
    m_iters = 0;
}

void StarThrottleLoop::writeConfig(json &j) {
    // j["max"] = max;
    // j["step"] = step;
    j["max_iters"] = max_iters;
}

void StarThrottleLoop::loadConfig(const json &j) {
    // if (j.contains("max"))
    //     max = j["max"];
    // if (j.contains("step"))
    //     step = j["step"];
    if (j.contains("max_iters"))
        max_iters = j["max_iters"];
}

void StarThrottleLoop::feedback(unsigned channel, double sign, bool last) {
    logger->trace("Received feedback {} on {} last? {}", sign, channel, last);
    m_done = last;
}

void StarThrottleLoop::init() {
    m_iters = 0;
    m_done = false;
    logger->debug("Init");
}

void StarThrottleLoop::execPart1() {
    logger->debug("ExecPart1");
    m_iters ++;
}

void StarThrottleLoop::execPart2() {
    logger->debug("ExecPart2, start");
    bool foundActive = false;
    for(unsigned id=0; id<keeper->getNumOfEntries(); id++) {
        FrontEnd *fe = keeper->getEntry(id).fe;
        if (fe->getActive()) {
            foundActive = true;
            unsigned rx = dynamic_cast<FrontEndCfg*>(fe)->getRxChannel();
            logger->trace("waitForFeedback on {} ...", rx);
            waitForFeedback(rx);
            logger->trace(" ... waitForFeedback complete on {}", rx);
        }
    }

    if(!foundActive) {
        logger->trace("No front-ends to wait foe");
        m_done = true;
    }

    if(!m_done) {
        if(m_iters >= max_iters) {
            m_done = true;
            logger->trace(" ... complete due to max iterations");
        } else {
            logger->trace(" ... continue {} of {} ...", m_iters, max_iters);
        }
    }

    logger->debug(" ... ExecPart2 done");
}

void StarThrottleLoop::end() {
    logger->debug("End");
}
