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

#include "LoopActionBase.h"
#include "FeedbackBase.h"

class Rd53a;

class Rd53aPixelFeedback : public LoopActionBase, public PixelFeedbackBase {
    public:
        Rd53aPixelFeedback();

        void writeConfig(json &j) override final;
        void loadConfig(json &j)  override final;

        void feedback(unsigned channel, Histo2d *h) override final;

    protected:
    private:

        class Impl;
        std::unique_ptr<Impl> m_impl;

        void addFeedback(unsigned ch);
        void writePixelCfg(Rd53a& fe);
        
        void init()      override final;
        void end()       override final;
        void execPart1() override final;
        void execPart2() override final;
};


#endif
