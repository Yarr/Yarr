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

class Rd53aGlobalFeedback : public LoopActionBase, public GlobalFeedbackReceiver {
    public:
        Rd53aGlobalFeedback();
        Rd53aGlobalFeedback(Rd53aReg Rd53aGlobalCfg::*ref);

        void writeConfig(json &j) override;
        void loadConfig(json &j) override;

        // TODO should probably register a single function
        void feedback(unsigned channel, double sign, bool last = false) override;
        void feedbackBinary(unsigned channel, double sign, bool last = false) override;
        void feedbackStep(unsigned channel, double sign, bool last = false) override;

    protected:
    private:
        Rd53aReg Rd53aGlobalCfg::*parPtr;
        std::string parName;
        int m_cur;

        std::map<unsigned, unsigned> m_values;
        std::map<unsigned, unsigned> m_localStep;
        std::map<unsigned, int> m_oldSign;
        std::map<unsigned, bool> m_doneMap;
        std::vector<unsigned> m_pixelReg;

        void writePar();
        bool allDone();
        
        void init() override;
        void end() override;
        void execPart1() override;
        void execPart2() override;
};


#endif
