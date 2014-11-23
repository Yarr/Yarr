/*
 * Authors: T. Heim <timon.heim@cern.ch>,
 * Date: 2014-Oct-16
 */

#ifndef FEI4GREGFEEDBACK_H
#define FEI4GREGFEEDBACK_H

#include <queue>
#include <mutex>
#include "LoopActionBase.h"

class Fei4GRegFeedbackBase : public LoopActionBase {
    public: 
        // TODO Feedback needs to handle multiple channels
        void feedback(double sign, bool last = false) {
            if (sign != oldSign)
                step = step/2;
            values.push(cur+(step*sign));
            std::cout << "--> Adding new value " << cur+(step*sign) << std::endl;
            oldSign = sign;
            m_done = last;
            fbMutex.unlock();
        }
   protected:
        std::mutex fbMutex;
        std::queue<unsigned> values;
        double oldSign;
        unsigned cur;

};


template<typename T, unsigned mOffset, unsigned bOffset, unsigned mask, bool msbRight>
class Fei4GRegFeedback : public Fei4GRegFeedbackBase {
    public:
        Fei4GRegFeedback(Field<T, mOffset, bOffset, mask, msbRight> Fei4GlobalCfg::*ref): parPtr(ref) { 
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
            if (cur < 50) m_done = true;
        }

        void writePar() {
            g_fe->writeRegister(parPtr, cur);
        }


        Field<T, mOffset, bOffset, mask, msbRight> Fei4GlobalCfg::*parPtr;
};

template<typename T, unsigned mOffset, unsigned bOffset, unsigned mask, bool msbRight>
Fei4GRegFeedbackBase* Fei4GRegFeedbackBuilder(Field<T, mOffset, bOffset, mask, msbRight> Fei4GlobalCfg::*ref) {
        return new Fei4GRegFeedback<T,mOffset,bOffset,mask,msbRight>(ref);
}

#endif
