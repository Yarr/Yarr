/*
 * Authors: T. Heim <timon.heim@cern.ch>,
 * Date: 2014-Oct-16
 */

#ifndef FEI4PARAMETERLOOP_H
#define FEI4PARAMETERLOOP_H

#include "LoopActionBase.h"

class Fei4ParameterLoop : public LoopActionBase {
    public:
        Fei4ParameterLoop();

        template<typename T, unsigned mOffset, unsigned bOffset, unsigned mask, bool msbRight>
        void setParamter(Field<T, mOffset, bOffset, mask, msbRight> Fei4GlobalCfg::*ref) {
            par = new Field<T, mOffset, bOffset, mask, msbRight>(ref);
        }
        
        void setRange(unsigned arg_min, unsigned arg_max, unsigned arg_step);
        
    private:
        void init();
        void end();
        void execPart1();
        void execPart2();

        unsigned min;
        unsigned step;
        unsigned max;
        unsigned cur;

        Field<uint16_t, 0, 0, 0, false> Fei4GlobalCfg::*par; 
};


#endif
