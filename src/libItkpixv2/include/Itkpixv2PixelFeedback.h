#ifndef ITKPIXV2PIXELFEEDBACK_H
#define ITKPIXV2PIXELFEEDBACK_H

// #################################
// # Author: Timon Heim
// # Email: timon.heim at cern.ch
// # Project: Yarr
// # Description: ITKPIXV2 Pixel Feedback Loopaction
// # Date: 07/2023
// ################################

#include "LoopActionBase.h"
#include "FeedbackBase.h"
#include "Itkpixv2.h"

class Itkpixv2PixelFeedback : public LoopActionBase, public PixelFeedbackReceiver {
    public:
        Itkpixv2PixelFeedback();

        void writeConfig(json &j) override;
        void loadConfig(const json &j) override;

        void feedback(unsigned channel, std::unique_ptr<Histo2d> h) override;
    protected:
    private:
        unsigned m_cur;
        bool m_rstPixelReg;
        int m_pixelReg;
        std::vector<int> m_steps;

        std::map<unsigned, std::unique_ptr<Histo2d>> m_fb;

        void addFeedback(unsigned ch);
        void writePixelCfg(Itkpixv2 *fe);

        void init() override;
        void end() override;
        void execPart1() override;
        void execPart2() override;
};

#endif
