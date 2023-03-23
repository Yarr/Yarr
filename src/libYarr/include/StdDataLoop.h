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
        void loadConfig(const json &config) override;

    private:
        //ClipBoard<RawDataContainer> *storage;
        unsigned counter;
        void init() override;
        void end() override;
        void execPart1() override;
        void execPart2() override;

        uint32_t n_triggersToReceive = 0;
        uint32_t n_triggersLostTolerance = 0; // allowed number of lost triggers
        uint32_t m_maxConsecutiveRxReads = 2;
        std::chrono::microseconds m_totalIterationTime{5000000};
        std::chrono::microseconds m_rxReadDelay{100};
        std::chrono::microseconds m_dataProcessingTime{100};
};

#endif


