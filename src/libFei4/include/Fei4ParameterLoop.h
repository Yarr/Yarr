/*
 * Authors: T. Heim <timon.heim@cern.ch>,
 * Date: 2014-Oct-16
 */

#ifndef FEI4PARAMETERLOOP_H
#define FEI4PARAMETERLOOP_H

#include "LoopActionBase.h"


class Fei4ParameterLoopBase : public LoopActionBase {
    public: 
        void setRange(unsigned arg_min, unsigned arg_max, unsigned arg_step) {
            min = arg_min;
            max = arg_max;
            step = arg_step;
        }
        
    protected:
        unsigned min;
        unsigned step;
        unsigned max;
};


template<typename T, unsigned mOffset, unsigned bOffset, unsigned mask, bool msbRight>
class Fei4ParameterLoop : public Fei4ParameterLoopBase {
    public:
        Fei4ParameterLoop(Field<T, mOffset, bOffset, mask, msbRight> Fei4GlobalCfg::*ref): parPtr(ref) { };
        
        
    private:
        void init() {
            m_done = false;
            cur = min;
            this->writePar();
        }

        void end() {}
        void execPart1() {
            if (verbose)
                std::cout << __PRETTY_FUNCTION__ << " : Parameter Loop at -> " << cur << std::endl;
            if (splitData)
                g_stat->set(this, cur);
        }

        void execPart2() {
            cur += step;
            if (cur > max) m_done = true;
            this->writePar();
        }

        void writePar() {
            g_fe->writeRegister(parPtr, cur);
        }

        unsigned cur;

        Field<T, mOffset, bOffset, mask, msbRight> Fei4GlobalCfg::*parPtr;
};

template<typename T, unsigned mOffset, unsigned bOffset, unsigned mask, bool msbRight>
Fei4ParameterLoopBase* Fei4ParameterLoopBuilder(Field<T, mOffset, bOffset, mask, msbRight> Fei4GlobalCfg::*ref) {
        return new Fei4ParameterLoop<T,mOffset,bOffset,mask,msbRight>(ref);
}

#endif
