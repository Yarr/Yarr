#ifndef STARCHANNELFEEDBACK_H
#define STARCHANNELFEEDBACK_H

// #################################
// # Author:
// # Email:
// # Project: Yarr
// # Description: Star Channel Feedback Loopaction
// # Comment: 
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

        void writeConfig(json &j);
        void loadConfig(json &j);

        void feedback(unsigned channel, std::unique_ptr<Histo2d> h);

    protected:
    private:
        unsigned m_cur;
        bool tuneLin;
        bool m_resetTdac;
        std::vector<unsigned> m_steps;

        std::map<unsigned, std::unique_ptr<Histo2d>> m_fb;

        void addFeedback(unsigned ch);
        void writeChannelCfg(StarChips *fe);

        void init();
        void end();
        void execPart1();
        void execPart2();

        int m_nRow;
        int m_nCol;
};


#endif
