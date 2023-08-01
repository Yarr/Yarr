/*
 * Authors: T. Heim <timon.heim@cern.ch>,
 * Date: 2015-Jul-21
 */

#include "StdDataGatherer.h"

#include <chrono>
#include <iostream>
#include <thread>
#include <algorithm>
#include <set>
#include <csignal>

#include "logging.h"

using Clock = std::chrono::steady_clock;

namespace {
    auto sdglog = logging::make_log("StdDataGatherer");
}

StdDataGatherer::StdDataGatherer() : LoopActionBase(LOOP_STYLE_DATA) {
    loopType = typeid(this);
    min = 0;
    max = 0;
    step = 1;
    counter = 0;
}

void StdDataGatherer::init() {
    m_done = false;
    killswitch = false;
    SPDLOG_LOGGER_TRACE(sdglog, "");
}

void StdDataGatherer::end() {
    SPDLOG_LOGGER_TRACE(sdglog, "");
}

void StdDataGatherer::execPart1() {
    SPDLOG_LOGGER_TRACE(sdglog, "");
    if (g_tx->getTrigEnable() == 0)
        SPDLOG_LOGGER_ERROR(sdglog, "Trigger is not enabled, will get stuck here!");

}

sig_atomic_t signaled = 0;

void StdDataGatherer::execPart2() {
    SPDLOG_LOGGER_TRACE(sdglog, "");
    unsigned count = 0;
    unsigned nAllRxReadIterations = 0;

    signaled = 0;
    signal(SIGINT, [](int signum){signaled = 1;});
    signal(SIGUSR1, [](int signum){signaled = 1;});

    //! initial wait before reading data
    std::this_thread::sleep_for(g_rx->getReadDelay());

    SPDLOG_LOGGER_WARN(sdglog, "IMPORTANT! Going into endless loop unless timelimit is set, interrupt with ^c (SIGINT)!");

    bool receivingRxData = true;
    while (receivingRxData) {
        std::vector<RawDataPtr> newData;
        std::map<uint32_t, std::unique_ptr<RawDataContainer>> rdcMap;
        
        newData =  g_rx->readData();
        nAllRxReadIterations++;

        // Read all data until buffer is empty
        while (newData.size() > 0 && count < 4096 && signaled == 0 && !killswitch) {
            if (newData.size() > 0) {
	        for (auto &dataChunk : newData) {
                    count += dataChunk->getSize();
                    for (unsigned &uid : keeper->getRxToId(dataChunk->getAdr())) {
                        if (rdcMap[uid] == nullptr) {
                            rdcMap[uid] = std::make_unique<RawDataContainer>(g_stat->record());
                        }

                        rdcMap[uid]->add(dataChunk);
                    }
                }
            }
	    // Push the accumulated chunks for processing
	    for (auto &[id, rdc] : rdcMap) {
	      rdc->stat.is_end_of_iteration = false;
	      keeper->getEntry(id).fe->clipRawData.pushData(std::move(rdc));
	    }
	    rdcMap.clear();
            // Wait a little bit to increase chance of new data having arrived
            std::this_thread::sleep_for(std::chrono::microseconds(1));
            newData =  g_rx->readData();
	    nAllRxReadIterations++;
        }

	// Push any remaining data for processing
        for (auto &[id, rdc] : rdcMap) {
	  rdc->stat.is_end_of_iteration = false;
	  keeper->getEntry(id).fe->clipRawData.pushData(std::move(rdc));
        }

	if (count == 0) {
          SPDLOG_LOGGER_INFO(sdglog, "\033[1m\033[31m--> Received {} words in {} iterations!\033[0m", count, nAllRxReadIterations);
        } else {
          SPDLOG_LOGGER_INFO(sdglog, "--> Received {} words in {} iterations!", count, nAllRxReadIterations);
        }

        count = 0;

        if (signaled == 1 || killswitch) {
	  SPDLOG_LOGGER_WARN(sdglog, "Caught interrupt, stopping data taking!");
	  SPDLOG_LOGGER_WARN(sdglog, "Abort might leave data in buffers!");
	  g_tx->toggleTrigAbort();
	}

        // Whether to execute another Rx cycle:                                                                                                                             
        receivingRxData = !g_tx->isTrigDone();

        // wait for the sampling time before the next Rx read cycle                                                                                                        
	if (receivingRxData) std::this_thread::sleep_for(g_rx->getReadInterval());
    }

    // the iteration end marker for the processing & analysis
    // send end-of-iteration empty container with LoopStatus::is_end_of_iteration = true
    LoopStatus loopStatusIterationEnd({0}, {LoopStyle::LOOP_STYLE_GLOBAL_FEEDBACK});
    loopStatusIterationEnd.is_end_of_iteration = true;
    for (unsigned id=0; id<keeper->getNumOfEntries(); id++) {
      std::unique_ptr<RawDataContainer> cIterEnd = std::make_unique<RawDataContainer>(std::move(loopStatusIterationEnd));
      keeper->getEntry(id).fe->clipRawData.pushData(std::move(cIterEnd));
      keeper->getEntry(id).fe->clipProcFeedback.reset();
    }

    m_done = true;
    counter++;
}
