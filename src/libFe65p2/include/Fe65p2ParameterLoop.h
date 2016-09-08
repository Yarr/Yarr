/*
 * Authors: T. Heim <timon.heim@cern.ch>
 * Date: 2016-May-16
 */

#ifndef FE65P2PARAMETERLOOP_H
#define FE65P2PARAMETERLOOP_H

#include <iostream>

#include "LoopActionBase.h"
#include "Fe65p2.h"

class Fe65p2ParameterLoop : public LoopActionBase {
    public:
        Fe65p2ParameterLoop(Fe65p2GlobalReg Fe65p2GlobalCfg::*reg);
        
        void setRange(unsigned arg_min, unsigned arg_max, unsigned arg_step);

    private:
        Fe65p2GlobalReg Fe65p2GlobalCfg::*m_reg;
        unsigned cur;

        void init();
        void end();
        void execPart1();
        void execPart2();

};

#endif
