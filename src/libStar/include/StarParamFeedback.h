#ifndef STAR_PARAM_FEEDBACK_H
#define STAR_PARAM_FEEDBACK_H

// #################################
// # Description: Star parameter feedback Loopaction
// ################################

#include <iostream>
#include <queue>
#include <mutex>

#include "LoopActionBase.h"
#include "FeedbackBase.h"
#include "StarChips.h"

/**
   Feedback loop action to configure a parameter from analysis.
 */
class StarParamFeedback : public LoopActionBase, public PixelFeedbackReceiver {
    public:
        StarParamFeedback();

        /// Store configuration
        void writeConfig(json &j) override;

        /// Load configuration
        void loadConfig(const json &j) override;

        /// Update trims for FrontEnd corresponding to channel
        void feedback(unsigned channel, std::unique_ptr<Histo2d> h) override;

    private:
        unsigned m_cur;
        std::string m_par;

        void writeChannelCfg(StarChips *fe);

        void init() override;
        void end() override;
        void execPart1() override;
        void execPart2() override;
};

#endif
