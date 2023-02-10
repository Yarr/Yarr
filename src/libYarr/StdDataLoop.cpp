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
    if (m_trigLoop != nullptr) n_triggers_to_receive = m_trigLoop->getTrigCnt();
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
    unsigned all_rawdata_count = 0;
    uint32_t done = 0;
    unsigned iterations = 0;


    // the RX channels that actually received data, and expect feedback
    std::set<uint32_t> activeChannels;

    // the counters for the feedback from data processors
    std::map<uint32_t, uint32_t> channelReceivedTriggersCnt;
    std::map<uint32_t, uint32_t> channelReceivedRRCnt;
    std::map<uint32_t, uint32_t> channelReceivedControlCnt; // HPR etc

    for (unsigned id=0; id<keeper->getNumOfEntries(); id++) {
        channelReceivedTriggersCnt[id] = 0;
        channelReceivedRRCnt[id]       = 0;
        channelReceivedControlCnt[id]      = 0;
    }

    // to keep track of max time for the iteration
    std::chrono::microseconds time_elapsed;
    std::chrono::time_point<Clock> time_start = Clock::now();
    
    // conditions to end the iteration: all triggers are received or out of time
    bool received_all_triggers = true;
    bool there_is_still_time = false;

    do {
        std::vector<RawDataPtr> newData;
        std::map<uint32_t, std::unique_ptr<RawDataContainer>> rdcMap;

        while (done == 0) {
            // Check if trigger is done
            done = g_tx->isTrigDone();
            // Read all data there is
            do {
                newData =  g_rx->readData();
                iterations++;
                if (newData.size() > 0) {
                    for (auto &dataChunk : newData) {
                        // dedicated variables for easier perf probing and gdb etc
                        auto rx_rawdata_size = dataChunk->getSize();
                        auto elink_id = dataChunk->getAdr();

                        all_rawdata_count += rx_rawdata_size;
                        for (unsigned &uid : keeper->getRxToId(elink_id)) {
                            if (rdcMap[uid] == nullptr) {
                                rdcMap[uid] = std::make_unique<RawDataContainer>(g_stat->record());
                            }

                            rdcMap[uid]->add(dataChunk); // accumulate data for a given Rx ID
                            activeChannels.insert(uid);
                        }
                    }
                }
            } while (newData.size() > 0);
        }

        // Gather rest of data after timeout (defined by controller)
        std::this_thread::sleep_for(g_rx->getWaitTime()); // HW controller delay
        do {
            //curCnt = g_rx->getCurCount();
            newData = g_rx->readData();
            iterations++;
            if (newData.size() > 0) {
                for (auto &dataChunk : newData) {
                    auto rx_rawdata_size = dataChunk->getSize();
                    auto elink_id = dataChunk->getAdr();

                    all_rawdata_count += rx_rawdata_size;
                    for (unsigned &uid : keeper->getRxToId(elink_id)) {
                        if (rdcMap[uid] == nullptr) {
                            rdcMap[uid] = std::make_unique<RawDataContainer>(g_stat->record());
                        }

                        rdcMap[uid]->add(dataChunk);
                        activeChannels.insert(uid);
                    }
                }
            }
        } while (newData.size() > 0 || g_rx->getCurCount() != 0);

        // push the currently received RX raw data for processing
        for (auto &[id, rdc] : rdcMap) {
            rdc->stat.is_end_of_iteration = false;
            keeper->getEntry(id).fe->clipRawData.pushData(std::move(rdc));
        }

        if (all_rawdata_count == 0) {
          SPDLOG_LOGGER_DEBUG(sdllog, "\033[1m\033[31m--> Received {} words in {} iterations!\033[0m", all_rawdata_count ,iterations);
        } else {
          SPDLOG_LOGGER_DEBUG(sdllog, "--> Received {} words in {} iterations!", all_rawdata_count ,iterations);
        }

        // if the feedback system has been set up -- work feedback-based
        if (feedbackFromRawDataProcessing != nullptr) {
            // check for any feedback from data processing
            bool received_feedback = false;
            do {
                received_feedback = false;

                // check all the active channels for feedback
                for (auto& chan_id : activeChannels) {
                    bool received_on_chan = feedbackFromRawDataProcessing->at(chan_id).waitNotEmptyOrDoneOrTimeout(m_dataProcessingTime);
                    if (!received_on_chan) continue;

                    received_feedback |= received_on_chan;
                    auto params = feedbackFromRawDataProcessing->at(chan_id).popData();

                    if (params->trigger_tag >=  0) channelReceivedTriggersCnt[chan_id] += 1;
                    else if (params->trigger_tag == -2) channelReceivedRRCnt[chan_id]  += 1;
                    else if (params->trigger_tag == -3) channelReceivedControlCnt[chan_id] += 1;
                    else {
                        SPDLOG_LOGGER_DEBUG(sdllog, "--> StdDataLoop::execPart2 feedback received an unexpected trigger tag {}", params->trigger_tag);
                    }
                }
            } while (received_feedback);

            // test whether all channels received all triggers
            received_all_triggers = true;
            for (auto &[id, received_triggers] : channelReceivedTriggersCnt) {
                SPDLOG_LOGGER_DEBUG(sdllog, "--> StdDataLoop::execPart2 : chan {} received {} triggers from {}", id, received_triggers, n_triggers_to_receive);

                bool the_chan_got_all_trigs = received_triggers >= n_triggers_to_receive;
                received_all_triggers &= the_chan_got_all_trigs;
                if (the_chan_got_all_trigs) activeChannels.erase(id);
            }
        } else {
            received_all_triggers = true; // no feedback system -- consider it's true
        }

        // test whether there is still time for this iteration
        time_elapsed =
            std::chrono::duration_cast<std::chrono::microseconds>(Clock::now() - time_start);
        there_is_still_time = time_elapsed.count() < m_totalIterationTime.count();

        SPDLOG_LOGGER_DEBUG(sdllog, "--> StdDataLoop::execPart2 : still time {} = {} < {} and all trigs = {}", there_is_still_time, time_elapsed.count(), m_totalIterationTime.count(), received_all_triggers);
    } while (!received_all_triggers && there_is_still_time);

    m_done = true;
    counter++;
}

void StdDataLoop::loadConfig(const json &config) {

	if (config.contains("total_iteration_time_us"))
		m_totalIterationTime = std::chrono::microseconds(config["total_iteration_time_us"]);

	if (config.contains("average_data_processing_time_us"))
		m_dataProcessingTime = std::chrono::microseconds(config["average_data_processing_time_us"]);

	SPDLOG_LOGGER_INFO(sdllog, "Configured StdDataLoop: average_data_processing_time_us: {}", m_dataProcessingTime.count());
}
