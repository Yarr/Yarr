/*
 * Authors: T. Heim <timon.heim@cern.ch>,
 * Date: 2014-Sep-27
 */

#ifndef STDDATALOOP_H
#define STDDATALOOP_H

#include "LoopActionBase.h"
#include "ClipBoard.h"
#include "RawData.h"

class StdDataLoop: public LoopActionBase {
    public:
        StdDataLoop();
        void connect(ClipBoard<RawData> *clipboard);
        
    private:
        ClipBoard<RawData> *storage;
        
        void init();
        void end();
        void execPart1();
        void execPart2();
};

#endif


