#ifndef RD53BPIXELFEEDBACK_H
#define RD53BPIXELFEEDBACK_H

// #################################
// # Author: Timon Heim
// # Email: timon.heim at cern.ch
// # Project: Yarr
// # Description: RD53B Pixel Feedback Loopaction
// # Date: 07/2020
// ################################

#include "LoopActionBase.h"
#include "FeedbackBase.h"
#include "Rd53b.h"

class Rd53bPixelFeedback : public LoopActionBase, public PixelFeedbackReceiver {
    public:
        Rd53bPixelFeedback();

        void writeConfig(json &j) override;
        void loadConfig(const json &j) override;

        void feedback(unsigned channel, std::unique_ptr<Histo2d> h) override;
    protected:
    private:
        unsigned m_cur;
        bool m_rstPixelReg;
        int m_pixelReg;
        std::vector<unsigned> m_steps;

        std::map<unsigned, std::unique_ptr<Histo2d>> m_fb;

        void addFeedback(unsigned ch);
        void writePixelCfg(Rd53b *fe);

        void init() override;
        void end() override;
        void execPart1() override;
        void execPart2() override;
};

#endif
