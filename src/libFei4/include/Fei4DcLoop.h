/*
 * Authors: T. Heim <timon.heim@cern.ch>,
 * Date: 2013-Oct-22
 */

#ifndef FEI4DCLOOP_H
#define FEI4DCLOOP_H

#include "LoopActionBase.h"
#include "Fei4.h"

enum DcLoop_Type {
    SINGLE_DC = 0x0,
    QUAD_DC = 0x1,
    OCTA_DC = 0x2,
    ALL_DC = 0x3
};

class Fei4DcLoop: public LoopActionBase {
    public:
        Fei4DcLoop();
       
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

