/*
 * Authors: T. Heim <timon.heim@cern.ch>,
 * Date: 2014-Sep-27
 */

#ifndef STDDATALOOP_H
#define STDDATALOOP_H

#include "LoopActionBase.h"
#include "StdDataAction.h"
#include "DataProcessor.h"
#include "ClipBoard.h"
#include "RawData.h"

class StdDataLoop: public LoopActionBase, public StdDataAction {
    public:
        StdDataLoop();
        //void connect(ClipBoard<RawDataContainer> *clipboard);

    private:
        //ClipBoard<RawDataContainer> *storage;
        unsigned counter;
        void init() override;
        void end() override;
        void execPart1() override;
        void execPart2() override;

        uint32_t ntriggersToReceive = 0;
};

#endif


