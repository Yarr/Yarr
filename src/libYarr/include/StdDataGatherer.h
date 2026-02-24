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

/**
 * Gather data.
 *
 * Loop over received data until stopped (via SIGINT).
 */
class StdDataGatherer: public LoopActionBase, public StdDataAction {
    public:
        StdDataGatherer();
        //   /** Connect to output data queue. */
        //void connect(ClipBoard<RawDataContainer> *clipboard);

        /** Stop gathering data. */
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
        bool m_passData;
        uint32_t m_maxConsecutiveRxReads = 2048;
        uint32_t m_maxRxReadSize = 10*1024*1024/4; // 100MB
};

#endif


