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

/**
 * Wait for completion of burst and collect data from RxCore.
 */
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

        uint32_t ntriggersToReceive = 0;
        std::chrono::microseconds m_maxIterationTime{5000000}; // in microseconds
        uint32_t m_maxConsecutiveRxReads = 2; // the same logic as in StdDataGatherer: we don't want to stuck in a continuous stream of Rx Data
        std::chrono::microseconds m_averageDataProcessingTime{100};
        uint32_t m_triggersLostTolerance = 0; // allowed number of lost triggers

        /// Record of stats per loop
        std::vector<std::array<float, 4>> m_stats;
};

#endif


