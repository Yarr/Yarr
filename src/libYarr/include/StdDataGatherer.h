/*
 * Authors: T. Heim <timon.heim@cern.ch>,
 * Date: 2015-Jul-21
 */

#ifndef STDDATALOOP_H
#define STDDATALOOP_H

#include "LoopActionBase.h"
#include "ClipBoard.h"
#include "RawData.h"
#include <signal.h>

// TODO should not be global, but don't know how to do it better
sig_atomic_t signaled;
void handle_sig(int param);

class StdDataGatherer: public LoopActionBase {
    public:
        StdDataGatherer();
        void connect(ClipBoard<RawDataContainer> *clipboard);
        
    private:
        ClipBoard<RawDataContainer> *storage;
        unsigned counter;
        void init();
        void end();
        void execPart1();
        void execPart2();

};

#endif


