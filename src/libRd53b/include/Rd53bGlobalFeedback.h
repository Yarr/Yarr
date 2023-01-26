#ifndef RD53BGLOBALFEEDBACK_H
#define RD53BGLOBALFEEDBACK_H

// #################################
// # Author: Timon Heim
// # Email: timon.heim at cern.ch
// # Project: Yarr
// # Description: RD53A Global Feedback Loopaction
// # Date: July 2020
// ################################

#include <queue>
#include <mutex>

#include "LoopActionBase.h"
#include "FeedbackBase.h"
#include "Rd53b.h"

class Rd53bGlobalFeedback : public LoopActionBase, public GlobalFeedbackReceiver {
    public:
        Rd53bGlobalFeedback();
        Rd53bGlobalFeedback(Rd53bRegDefault Rd53bGlobalCfg::*ref);

        void writeConfig(json &j) override;
        void loadConfig(const json &j) override;

        // TODO should probably register a single function
        void feedback(unsigned channel, double sign, bool last = false) override;
        void feedbackBinary(unsigned channel, double sign, bool last = false) override;
        void feedbackStep(unsigned channel, double sign, bool last = false) override;

    protected:
    private:
        Rd53bRegDefault Rd53bGlobalCfg::*parPtr;
        std::string parName;
        int m_cur;

        std::map<unsigned, unsigned> m_values;
        std::map<unsigned, unsigned> m_localStep;
        std::map<unsigned, int> m_oldSign;

        bool m_rstPixelReg;
        int m_pixelReg;

        void writePar();
        
        void init() override;
        void end() override;
        void execPart1() override;
        void execPart2() override;
};


#endif
