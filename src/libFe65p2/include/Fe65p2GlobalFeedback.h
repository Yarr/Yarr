/*
 * Authors: T. Heim <timon.heim@cern.ch>,
 * Date: 2014-May-17
 */

#ifndef FE65P2GLOBALFEEDBACK_H
#define FE65P2GLOBALFEEDBACK_H

#include <queue>
#include <mutex>

#include "LoopActionBase.h"
#include "FeedbackBase.h"
#include "Fe65p2.h"

class Fe65p2GlobalFeedback : public LoopActionBase, public GlobalFeedbackBase {
    public:
        Fe65p2GlobalFeedback(Fe65p2GlobalReg Fe65p2GlobalCfg::*reg);
        
        void feedbackBinary(unsigned channel, double sign, bool last = false);
        void feedback(unsigned channel, double sign, bool last = false);
        
    private:
        Fe65p2GlobalReg Fe65p2GlobalCfg::*m_reg;
        
        std::mutex fbMutex;
        std::map<unsigned, unsigned> values;
		std::map<unsigned, unsigned> localStep;
		std::map<unsigned, double> oldSign;
        unsigned cur;
        
        void init();
        void end();
        void execPart1();
        void execPart2();
};

#endif
