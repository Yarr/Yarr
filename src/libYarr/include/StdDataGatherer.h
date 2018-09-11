/*
 * Authors: T. Heim <timon.heim@cern.ch>,
 * Date: 2015-Jul-21
 */

#ifndef STDDATAGATHERER_H
#define STDDATAGATHERER_H

#include "LoopActionBase.h"
#include "StdDataAction.h"
#include "ClipBoard.h"
#include "RawData.h"
#include <signal.h>

class StdDataGatherer: public LoopActionBase, public StdDataAction {
    public:
        StdDataGatherer();
        //void connect(ClipBoard<RawDataContainer> *clipboard);

        void kill() {
            killswitch = true;
        }
    private:
        //ClipBoard<RawDataContainer> *storage;
        unsigned counter;
        void init();
        void end();
        void execPart1();
        void execPart2();
        bool killswitch;
};

#endif


