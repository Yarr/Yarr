/*
 * Authors: T. Heim <timon.heim@cern.ch>,
 * Date: 2014-Oct-16
 */

#ifndef FEI4PARAMETERLOOP_H
#define FEI4PARAMETERLOOP_H

#include "LoopActionBase.h"

enum LOOP_PAR {
    VCAL_PAR
};

class Fei4ParameterLoop : public LoopActionBase {
    public:
        Fei4ParameterLoop();

        void setParameter(enum LOOP_PAR x) {
            par = x;
        }
        
        void setRange(unsigned arg_min, unsigned arg_max, unsigned arg_step);
        
    private:
        void init();
        void end();
        void execPart1();
        void execPart2();

        void writePar();

        enum LOOP_PAR par;
        unsigned min;
        unsigned step;
        unsigned max;
        unsigned cur;
};


#endif
