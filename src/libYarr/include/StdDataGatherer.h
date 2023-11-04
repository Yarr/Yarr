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

class StdDataGatherer: public LoopActionBase, public StdDataAction {
    public:
        StdDataGatherer();
        //void connect(ClipBoard<RawDataContainer> *clipboard);

        void kill() {
            killswitch = true;
        }

        void loadConfig(const json &config) override;

    private:
        //ClipBoard<RawDataContainer> *storage;
        unsigned counter;
        void init() override;
        void end() override;
        void execPart1() override;
        void execPart2() override;
        bool killswitch;

        uint32_t m_maxConsecutiveRxReads = 4096;
};

#endif


