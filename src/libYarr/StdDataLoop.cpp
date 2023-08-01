/*
 * Authors: T. Heim <timon.heim@cern.ch>,
 * Date: 2014-Sep-27
 */

#include "StdDataLoop.h"

#include <chrono>
#include <iostream>
#include <thread>
#include <algorithm>
#include <set>

#include "logging.h"

using Clock = std::chrono::steady_clock;

namespace {
    auto sdllog = logging::make_log("StdDataLoop");
}

StdDataLoop::StdDataLoop() : LoopActionBase(LOOP_STYLE_DATA) {
    loopType = typeid(this);
    min = 0;
    max = 0;
    step = 1;
    counter = 0;
}

void StdDataLoop::init() {
    m_done = false;
    auto trigAction = keeper->getTriggerAction();
    if (trigAction != nullptr) ntriggersToReceive = trigAction->getExpEvents() - g_rx->getTriggersLostTolerance();
    SPDLOG_LOGGER_TRACE(sdllog, "");
}

void StdDataLoop::end() {
    SPDLOG_LOGGER_TRACE(sdllog, "");
}

void StdDataLoop::execPart1() {
    SPDLOG_LOGGER_TRACE(sdllog, "");
    if (g_tx->getTrigEnable() == 0)
        SPDLOG_LOGGER_ERROR(sdllog, "Trigger is not enabled, will get stuck here!");

}

void StdDataLoop::execPart2() {
    SPDLOG_LOGGER_TRACE(sdllog, "");
    unsigned allRawDataCount = 0;
    unsigned nAllRxReadIterations = 0;
    uint32_t triggerIsDone = 0;

    // the RX channels that actually received data, and expect feedback
    std::set<uint32_t> activeChannels;

    // the counters for the feedback from data processors
    std::map<uint32_t, uint32_t> channelReceivedTriggersCnt;
    std::map<uint32_t, uint32_t> channelReceivedRRCnt;
    std::map<uint32_t, uint32_t> channelReceivedControlCnt; // HPR etc
    std::map<uint32_t, uint32_t> channelReceivedPacketSize;
    std::map<uint32_t, uint32_t> channelReceivedNClusters;

    for (unsigned id=0; id<keeper->getNumOfEntries(); id++) {
        channelReceivedTriggersCnt[id] = 0;
        channelReceivedRRCnt[id]       = 0;
        channelReceivedControlCnt[id]  = 0;
    }

    // to keep track of max time for the iteration
    std::chrono::microseconds timeElapsed;
    std::chrono::time_point<Clock> timeStart = Clock::now();
    
    // conditions to end the iteration: all triggers are received or out of time
    bool receivedAllTriggers = true;
    bool thereIsStillTime = false;

    //! initial wait before reading data                                                                                                                                    
    std::this_thread::sleep_for(g_rx->getReadDelay());

    SPDLOG_LOGGER_DEBUG(sdllog, "Reading Rx data...");

    //! Rx read cycle: read the data from RxCore, push to data processors, check feedback
    bool receivingRxData = true;
    while (receivingRxData) {
        std::vector<RawDataPtr> newData;

        // accumulate the RawData chunks per each elink from N reads from HW controller RS
        // at the end, push the RawDataContainers for processing
        std::map<uint32_t, std::unique_ptr<RawDataContainer>> rdcMap;
        unsigned nReadsInCurrentRxCycle = 0;
        do {
            newData = g_rx->readData();  // read the data from HW controller
            nAllRxReadIterations++;
            nReadsInCurrentRxCycle++;

            if (newData.size() > 0) {
                for (auto &dataChunk : newData) {
                    auto rxRawDataSize = dataChunk->getSize(); // variables for probing/debugging
                    auto elinkId = dataChunk->getAdr();

                    allRawDataCount += rxRawDataSize;
                    for (unsigned &uid : keeper->getRxToId(elinkId)) {
                        if (rdcMap[uid] == nullptr) {
                            rdcMap[uid] = std::make_unique<RawDataContainer>(g_stat->record());
                        }

                        rdcMap[uid]->add(dataChunk);
                        //activeChannels.insert(uid);
                    }
                }

                // push the accumulated chunks for processing, if N RX reads > threshold
                if (nReadsInCurrentRxCycle > g_rx->getMaxConsecutiveRxReads()) {
                    for (auto &[id, rdc] : rdcMap) {
                        rdc->stat.is_end_of_iteration = false;
                        keeper->getEntry(id).fe->clipRawData.pushData(std::move(rdc));
                        activeChannels.insert(id);
                    }
                    nReadsInCurrentRxCycle = 0; rdcMap.clear();
                }
            }
        }
        while (newData.size() > 0 || g_rx->getCurCount() != 0);

        // if there is anything left to process -- push it
        for (auto &[id, rdc] : rdcMap) {
            rdc->stat.is_end_of_iteration = false;
            keeper->getEntry(id).fe->clipRawData.pushData(std::move(rdc));
            activeChannels.insert(id);
        }

        if (allRawDataCount == 0) {
          SPDLOG_LOGGER_DEBUG(sdllog, "\033[1m\033[31m--> Received {} words in {} iterations!\033[0m", allRawDataCount, nAllRxReadIterations);
        } else {
          SPDLOG_LOGGER_DEBUG(sdllog, "--> Received {} words in {} iterations!", allRawDataCount, nAllRxReadIterations);
        }

        // check for any feedback from data processing
        bool receivedFeedbackSomewhere = false;
		// monitoring for debugging:
		uint32_t iterationNrrs  = 0;
		uint32_t iterationNhprs = 0;
		uint32_t iterationNerrs = 0;
        do {
            receivedFeedbackSomewhere = false;

            // check the active channels for feedback
            for (auto& chan_id : activeChannels) {
                // pull all the currently available feedback from this channel
	      while(bool receivedOnChan = keeper->getEntry(chan_id).fe->clipProcFeedback.waitNotEmptyOrDoneOrTimeout(g_rx->getAverageDataProcessingTime())) {
                    receivedFeedbackSomewhere |= receivedOnChan;
                    auto params = keeper->getEntry(chan_id).fe->clipProcFeedback.popData();

                    if (params->trigger_tag >=  0) {
                        channelReceivedTriggersCnt[chan_id] += 1;
                        channelReceivedPacketSize[chan_id] += params->packet_size;
                        channelReceivedNClusters[chan_id]  += params->n_clusters;
                    }
                    else if (params->trigger_tag == PROCESSING_FEEDBACK_TRIGGER_TAG_RR) {
                        channelReceivedRRCnt[chan_id]  += 1;
                        iterationNrrs++;
                    }
                    else if (params->trigger_tag == PROCESSING_FEEDBACK_TRIGGER_TAG_Control) {
                        channelReceivedControlCnt[chan_id] += 1;
                        iterationNhprs++;
                    }
                    else { // SPDLOG_LOGGER_DEBUG(sdllog, "--> StdDataLoop::execPart2 feedback received an unexpected trigger tag {}", params->trigger_tag);
                        iterationNerrs++;
                    }
                }
            }
        } while (receivedFeedbackSomewhere);

        // test whether all channels received all triggers
        unsigned channelsWithAllTrigsN = 0; // 
        uint32_t nAllReceivedTriggersSoFar = 0;
        for (auto &[id, receivedTriggers] : channelReceivedTriggersCnt) {
            //SPDLOG_LOGGER_DEBUG(sdllog, "--> StdDataLoop::execPart2 : chan {} received {} triggers from {}", id, received_triggers, ntriggersToReceive);
			nAllReceivedTriggersSoFar += receivedTriggers;

            if (receivedTriggers >= ntriggersToReceive) {
                //activeChannels.erase(id); // ok, don't erase a channel - it looks like we receive some random 1-2 triggers here and there
                channelsWithAllTrigsN += 1;
            }
        }
        receivedAllTriggers = channelsWithAllTrigsN >= keeper->getNumOfEntries();

        // test whether there is still time for this iteration
        timeElapsed =
            std::chrono::duration_cast<std::chrono::microseconds>(Clock::now() - timeStart);
        thereIsStillTime = timeElapsed.count() < g_rx->getWaitTime().count(); // up to HW controller latency

        // Check if trigger is done
        triggerIsDone = g_tx->isTrigDone();

        // Whether to execute another Rx cycle:
        receivingRxData = !triggerIsDone || (thereIsStillTime && !receivedAllTriggers);
        SPDLOG_LOGGER_DEBUG(sdllog, "one more Rx cycle: {} -- still time {} = {} < {} and all trigs = {} (n channels w all trigs = {}, n trigs = {}, n rrs hprs errs = {} {} {})",
			    receivingRxData, thereIsStillTime, timeElapsed.count(), g_rx->getWaitTime().count(), receivedAllTriggers,
            channelsWithAllTrigsN, nAllReceivedTriggersSoFar, iterationNrrs, iterationNhprs, iterationNerrs);

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

    // report the average channel occupancy data
    for (auto &[id, receivedTriggers] : channelReceivedTriggersCnt) {
        float avSizes    = ((float) channelReceivedPacketSize[id]) / ((float) receivedTriggers);
        float avClusters = ((float) channelReceivedNClusters[id])  / ((float) receivedTriggers);
        SPDLOG_LOGGER_DEBUG(sdllog, "channel {} received {} triggers, in packets sizes {} with {} clusters", id, receivedTriggers, avSizes, avClusters);
    }

    m_done = true;
    counter++;
}
