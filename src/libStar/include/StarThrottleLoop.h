#ifndef STARTHROTTLELOOP_H
#define STARTHROTTLELOOP_H

// #################################
// # Author:
// # Email:
// # Project: Yarr
// # Description: Star Global Feedback Loop action
// # Comment: 
// # Date: Oct 2018
// ################################


#include <iostream>
#include <queue>
#include <mutex>

#include "LoopActionBase.h"
#include "FeedbackBase.h"
#include "StarChips.h"

class StarThrottleLoop : public LoopActionBase, public GlobalFeedbackReceiver {
    public:
        StarThrottleLoop();

        void writeConfig(json &j) override;
        void loadConfig(const json &j) override;

        // TODO should probably register a single function
        void feedback(unsigned channel, double sign, bool last = false) override;
        void feedbackBinary(unsigned channel, double sign, bool last) override {}

    private:
        unsigned m_iters, max_iters;

        void init();
        void end();
        void execPart1();
        void execPart2();
};


#endif
