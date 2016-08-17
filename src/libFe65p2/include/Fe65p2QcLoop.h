/*
 * Authors: T. Heim <timon.heim@cern.ch>,
 * Date: 2016-Mar-30
 */

#ifndef FE65P2QCLOOP_H
#define FE65P2QCLOOP_H

#include "LoopActionBase.h"
#include "Fe65p2.h"

class Fe65p2QcLoop : public LoopActionBase {
    public:
        Fe65p2QcLoop();

    private:
        unsigned m_mask;
        unsigned m_cur;

        void init();
        void end();
        void execPart1();
        void execPart2();
};

#endif
