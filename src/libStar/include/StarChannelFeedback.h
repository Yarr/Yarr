#ifndef STARCHANNELFEEDBACK_H
#define STARCHANNELFEEDBACK_H

// #################################
// # Project: Yarr
// # Description: Star Channel Feedback Loopaction
// # Date: Nov 2019
// ################################


#include <iostream>
#include <queue>
#include <mutex>

#include "LoopActionBase.h"
#include "FeedbackBase.h"
#include "StarChips.h"

class StarChannelFeedback : public LoopActionBase, public PixelFeedbackReceiver {
    public:
        StarChannelFeedback();

        void writeConfig(json &j) override;
        void loadConfig(json &j) override;

        void feedback(unsigned channel, std::unique_ptr<Histo2d> h) override;

    private:
        unsigned m_cur;
        bool m_resetTdac;
        std::vector<unsigned> m_steps;

        std::map<unsigned, std::unique_ptr<Histo2d>> m_fb;

        void addFeedback(unsigned ch);
        void writeChannelCfg(StarChips *fe);

        void init() override;
        void end() override;
        void execPart1() override;
        void execPart2() override;
};


#endif
