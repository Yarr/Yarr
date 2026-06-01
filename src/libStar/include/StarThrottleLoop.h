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
        StarThrottleLoop(std::string subRegName);
        StarThrottleLoop(Register StarCfg::*ref);

        void writeConfig(json &j) override;
        void loadConfig(const json &j) override;

        // TODO should probably register a single function
        void feedback(unsigned channel, double sign, bool last = false) override;
        void feedbackBinary(unsigned channel, double sign, bool last) override {}

    private:
        Register StarCfg::*parPtr = nullptr;
        SubRegister* StarCfg::*subRegPtr = nullptr;
        std::string m_subRegName;
        unsigned m_iters = 0, m_trigs = 0, max_iters = 0, m_curStep = 0;

        std::mutex m_fbMutex;
        std::map<unsigned, int> m_values;
        std::map<unsigned, int> m_localStep;
        std::map<unsigned, int> m_oldSign;
        std::map<unsigned, bool> m_doneMap;

        bool allDone();
        
        void init() override;
        void end() override;
        void execPart1() override;
        void execPart2() override;
};


#endif
