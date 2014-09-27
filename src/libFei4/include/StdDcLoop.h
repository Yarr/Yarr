/*
 * Authors: T. Heim <timon.heim@cern.ch>,
 * Date: 2013-Oct-22
 */

#ifndef STDDCLOOP_H
#define STDDCLOOP_H

#include "LoopActionBase.h"
#include "Fei4.h"

enum DcLoop_Type {
    SINGLE_DC = 0x0,
    QUAD_DC = 0x1,
    OCTA_DC = 0x2,
    ALL_DC = 0x3
};

class StdDcLoop: public LoopActionBase {
    public:
        StdDcLoop();
       
        void setMode(DcLoop_Type mode);
        DcLoop_Type getMode();

    private:
        DcLoop_Type m_mode;
        int m_col, m_colStart, m_colEnd;

        void init();
        void end();
        void execPart1();
        void execPart2();
};

#endif

