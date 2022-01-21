#ifndef RD53APIXELFEEDBACK_H
#define RD53APIXELFEEDBACK_H

// #################################
// # Author: Timon Heim
// # Email: timon.heim at cern.ch
// # Project: Yarr
// # Description: RD53A Pixel Feedback Loopaction
// # Comment: 
// # Date: April 2018
// ################################


#include <iostream>
#include <queue>
#include <mutex>

#include "LoopActionBase.h"
#include "FeedbackBase.h"
#include "Rd53a.h"

class Rd53aPixelFeedback : public LoopActionBase, public PixelFeedbackReceiver {
    public:
        Rd53aPixelFeedback();

        void writeConfig(json &j) override;
        void loadConfig(const json &j) override;

        void feedback(unsigned channel, std::unique_ptr<Histo2d> h) override;

    protected:
    private:
        unsigned m_cur;
        bool tuneLin;
        bool tuneDiff;
        bool m_resetTdac;
        std::vector<unsigned> m_steps;

        std::map<unsigned, std::unique_ptr<Histo2d>> m_fb;

        void addFeedback(unsigned ch);
        void writePixelCfg(Rd53a *fe);

        void init() override;
        void end() override;
        void execPart1() override;
        void execPart2() override;
};


#endif
