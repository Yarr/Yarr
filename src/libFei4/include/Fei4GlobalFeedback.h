/*
 * Authors: T. Heim <timon.heim@cern.ch>,
 * Date: 2014-Oct-16
 */

#ifndef FEI4GLOBALFEEDBACK_H
#define FEI4GLOBALFEEDBACK_H

#include <queue>
#include <mutex>
#include "LoopActionBase.h"

class Fei4GlobalFeedbackBase : public LoopActionBase {
    public: 
        // TODO Feedback needs to handle multiple channels
        void feedback(double sign, bool last = false) {
            if (sign != oldSign) {
                oldSign = 0;
                step = step/2;
            }
            int val = (cur+(step*sign));
            if (val < 0) val = 0;
            values.push(cur+(step*sign));
            std::cout << "--> Adding new value " << cur+(step*sign) << std::endl;
            m_done = last;
            if (val < 50) m_done = true;
            fbMutex.unlock();
        }
        
        void feedbackBinary(double sign, bool last = false) {
            int val = (cur+(step*sign));
            if (val < 0) val = 0;
            values.push(val);
            std::cout << "--> Adding new value " << val << std::endl;
            step = step/2;
            m_done = last;
            if (step == 1) m_done = true;
            fbMutex.unlock();
        }
   protected:
        std::mutex fbMutex;
        std::queue<unsigned> values;
        double oldSign;
        unsigned cur;

};


template<typename T, unsigned mOffset, unsigned bOffset, unsigned mask, bool msbRight>
class Fei4GlobalFeedback : public Fei4GlobalFeedbackBase {
    public:
        Fei4GlobalFeedback(Field<T, mOffset, bOffset, mask, msbRight> Fei4GlobalCfg::*ref): parPtr(ref) { 
            loopType = typeid(this);
            oldSign = -1;
        };
        
        
    private:
        void init() {
            m_done = false;
            cur = max;
            this->writePar();
        }

        void end() {}
        void execPart1() {
            g_stat->set(this, cur);
            fbMutex.try_lock();
        }

        void execPart2() {
            fbMutex.lock();
            cur = values.front();
            values.pop();
            std::cout << "--> Received feedback: " << cur << std::endl;
            this->writePar();
        }

        void writePar() {
            g_fe->writeRegister(parPtr, cur);
        }


        Field<T, mOffset, bOffset, mask, msbRight> Fei4GlobalCfg::*parPtr;
};

template<typename T, unsigned mOffset, unsigned bOffset, unsigned mask, bool msbRight>
Fei4GlobalFeedbackBase* Fei4GlobalFeedbackBuilder(Field<T, mOffset, bOffset, mask, msbRight> Fei4GlobalCfg::*ref) {
        return new Fei4GlobalFeedback<T,mOffset,bOffset,mask,msbRight>(ref);
}

#endif
