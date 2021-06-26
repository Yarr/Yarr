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
        void loadConfig(json &j);

        // TODO should probably register a single function
        void feedback(unsigned channel, double sign, bool last = false) override;
        void feedbackBinary(unsigned channel, double sign, bool last) {};

    private:
        Register StarCfg::*parPtr;
        SubRegister* StarCfg::*subRegPtr;
        std::string m_subRegName;
        unsigned m_iters, m_trigs, max_iters, m_curStep;

        std::mutex m_fbMutex;
        std::map<unsigned, int> m_values;
        std::map<unsigned, int> m_localStep;
        std::map<unsigned, int> m_oldSign;
        std::map<unsigned, bool> m_doneMap;

        bool allDone();
        
        void init();
        void end();
        void execPart1();
        void execPart2();
};


#endif
