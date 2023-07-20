#ifndef ITKPIXV2GLOBALFEEDBACK_H
#define ITKPIXV2GLOBALFEEDBACK_H

// #################################
// # Author: Timon Heim
// # Email: timon.heim at cern.ch
// # Project: Yarr
// # Description: ItkPixV2 Global Feedback Loopaction
// # Date: July 2020
// ################################

#include <queue>
#include <mutex>

#include "LoopActionBase.h"
#include "FeedbackBase.h"
#include "Itkpixv2.h"

class Itkpixv2GlobalFeedback : public LoopActionBase, public GlobalFeedbackReceiver {
    public:
        Itkpixv2GlobalFeedback();
        Itkpixv2GlobalFeedback(Itkpixv2RegDefault Itkpixv2GlobalCfg::*ref);

        void writeConfig(json &j) override;
        void loadConfig(const json &j) override;

        // TODO should probably register a single function
        void feedback(unsigned channel, double sign, bool last = false) override;
        void feedbackBinary(unsigned channel, double sign, bool last = false) override;
        void feedbackStep(unsigned channel, double sign, bool last = false) override;

    protected:
    private:
        Itkpixv2RegDefault Itkpixv2GlobalCfg::*parPtr;
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
