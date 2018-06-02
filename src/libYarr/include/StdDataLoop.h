/*
 * Authors: T. Heim <timon.heim@cern.ch>,
 * Date: 2014-Sep-27
 */

#ifndef STDDATALOOP_H
#define STDDATALOOP_H

#include "LoopActionBase.h"
#include "StdDataAction.h"
#include "ClipBoard.h"
#include "RawData.h"
#include <signal.h>

class StdDataLoop: public LoopActionBase, public StdDataAction {
    public:
        StdDataLoop();
        //void connect(ClipBoard<RawDataContainer> *clipboard);
        
    private:
        //ClipBoard<RawDataContainer> *storage;
        unsigned counter;
        void init();
        void end();
        void execPart1();
        void execPart2();
};

#endif


