/*
 * Authors: T. Heim <timon.heim@cern.ch>,
 * Date: 2014-Oct-16
 */

#ifndef FEI4PARAMETERLOOP_H
#define FEI4PARAMETERLOOP_H

#include "Fei4.h"
#include "LoopActionBase.h"

class Fei4ParameterLoop : public LoopActionBase{
    public:
        Fei4ParameterLoop() {
            loopType = typeid(this);
        }

        Fei4ParameterLoop(Fei4Register Fei4GlobalCfg::*ref) : parPtr(ref){ 
            loopType = typeid(this);
        };

        void setRange(unsigned arg_min, unsigned arg_max, unsigned arg_step) {
            min = arg_min;
            max = arg_max;
            step = arg_step;
        }


    private:
        void init() {
            m_done = false;
            cur = min;
            this->writePar();
        }

        void end() {}
        void execPart1() {
            if (verbose)
                std::cout << " : Parameter Loop at -> " << cur << std::endl;
            g_stat->set(this, cur);
        }

        void execPart2() {
            cur += step;
            if ((int)cur > max) m_done = true;
            this->writePar();
        }

        void writePar() {
            keeper->globalFe<Fei4>()->writeRegister(parPtr, cur);
            while(!g_tx->isCmdEmpty());
        }

        unsigned cur;

        Fei4Register Fei4GlobalCfg::*parPtr;


};

#endif
