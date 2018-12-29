#ifndef RD53AGLOBALFEEDBACK_H
#define RD53AGLOBALFEEDBACK_H

// #################################
// # Author: Timon Heim
// # Email: timon.heim at cern.ch
// # Project: Yarr
// # Description: RD53A Global Feedback Loopaction
// # Comment: 
// # Date: April 2018
// ################################


#include <iostream>
#include <queue>
#include <mutex>

#include "LoopActionBase.h"
#include "FeedbackBase.h"
#include "Rd53a.h"

class Rd53aGlobalFeedback : public LoopActionBase, public GlobalFeedbackBase {
    public:
        Rd53aGlobalFeedback();
        Rd53aGlobalFeedback(Rd53aReg Rd53aGlobalCfg::*ref);

        void writeConfig(json &j);
        void loadConfig(json &j);

        // TODO should probably register a single function
        void feedback(unsigned channel, double sign, bool last = false);
        void feedbackBinary(unsigned channel, double sign, bool last = false);
        void feedbackStep(unsigned channel, double sign, bool last = false);

    protected:
    private:
        Rd53aReg Rd53aGlobalCfg::*parPtr;
        std::string parName;
        int m_cur;

        std::mutex m_fbMutex;
        std::map<unsigned, unsigned> m_values;
        std::map<unsigned, unsigned> m_localStep;
        std::map<unsigned, int> m_oldSign;
        std::map<unsigned, bool> m_doneMap;
        std::vector<unsigned> m_pixelReg;

        void writePar();
        bool allDone();
        
        void init();
        void end();
        void execPart1();
        void execPart2();
};


#endif
