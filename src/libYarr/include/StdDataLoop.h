/*
 * Authors: T. Heim <timon.heim@cern.ch>,
 * Date: 2014-Sep-27
 */

#ifndef STDDATALOOP_H
#define STDDATALOOP_H

#include "LoopActionBase.h"
#include "StdDataAction.h"
#include "DataProcessor.h"
#include "StdTriggerAction.h"
#include "ClipBoard.h"
#include "RawData.h"

class StdDataLoop: public LoopActionBase, public StdDataAction {
    public:
        StdDataLoop();
        //void connect(ClipBoard<RawDataContainer> *clipboard);
        void connect(std::shared_ptr<StdTriggerAction> trigLoop) {m_trigLoop = trigLoop;};
        void loadConfig(const json &config) override;

    protected:
        std::shared_ptr<StdTriggerAction> m_trigLoop = nullptr;

    private:
        //ClipBoard<RawDataContainer> *storage;
        unsigned counter;
        void init() override;
        void end() override;
        void execPart1() override;
        void execPart2() override;

        uint32_t n_triggers_to_receive = 0;
        uint32_t m_maxConsecutiveRxReads = 2;
        std::chrono::microseconds m_totalIterationTime{5000000};
        std::chrono::microseconds m_dataProcessingTime{100};
};

#endif


