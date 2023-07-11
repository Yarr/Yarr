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
    if (trigAction != nullptr) n_triggersToReceive = trigAction->getTrigCnt()*trigAction->getTrigMulti() - n_triggersLostTolerance;
    SPDLOG_LOGGER_TRACE(sdllog, "");
    SPDLOG_LOGGER_INFO(sdllog, "Number of triggers to receive={}",n_triggersToReceive);
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
    unsigned n_all_rx_read_iterations = 0;
    uint32_t trigger_is_done = 0;

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
    std::chrono::microseconds time_elapsed;
    std::chrono::time_point<Clock> time_start = Clock::now();
    
    // conditions to end the iteration: all triggers are received or out of time
    bool received_all_triggers = true;
    bool there_is_still_time = false;

    //! wait the typical latency of the HW controller (network for Netio etc)
    std::this_thread::sleep_for(g_rx->getWaitTime()); // HW controller latency

    SPDLOG_LOGGER_DEBUG(sdllog, "Reading Rx data...");

    //! Rx read cycle: read the data from RxCore, push to data processors, check feedback
    bool receiving_rx_data = true;
    while (receiving_rx_data) {
        std::vector<RawDataPtr> newData;

        // accumulate the RawData chunks per each elink from N reads from HW controller RS
        // at the end, push the RawDataContainers for processing
        std::map<uint32_t, std::unique_ptr<RawDataContainer>> rdcMap;
        unsigned n_reads_in_current_rx_cycle = 0;
        do {
            newData = g_rx->readData();  // read the data from HW controller
            n_all_rx_read_iterations++;
            n_reads_in_current_rx_cycle++;

            if (newData.size() > 0) {
                for (auto &dataChunk : newData) {
                    auto rx_rawdata_size = dataChunk->getSize(); // variables for probing/debugging
                    auto elink_id = dataChunk->getAdr();

                    all_rawdata_count += rx_rawdata_size;
                    for (unsigned &uid : keeper->getRxToId(elink_id)) {
                        if (rdcMap[uid] == nullptr) {
                            rdcMap[uid] = std::make_unique<RawDataContainer>(g_stat->record());
                        }

                        rdcMap[uid]->add(dataChunk);
                        //activeChannels.insert(uid);
                    }
                }

                // push the accumulated chunks for processing, if N RX reads > threshold
                if (n_reads_in_current_rx_cycle > m_maxConsecutiveRxReads) {
                    for (auto &[id, rdc] : rdcMap) {
                        rdc->stat.is_end_of_iteration = false;
                        keeper->getEntry(id).fe->clipRawData.pushData(std::move(rdc));
                        activeChannels.insert(id);
                    }
                    n_reads_in_current_rx_cycle = 0; rdcMap.clear();
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

        if (all_rawdata_count == 0) {
          SPDLOG_LOGGER_DEBUG(sdllog, "\033[1m\033[31m--> Received {} words in {} iterations!\033[0m", all_rawdata_count, n_all_rx_read_iterations);
        } else {
          SPDLOG_LOGGER_DEBUG(sdllog, "--> Received {} words in {} iterations!", all_rawdata_count, n_all_rx_read_iterations);
        }

        // check for any feedback from data processing
        bool received_feedback_somewhere = false;
		// monitoring for debugging:
		uint32_t iteration_n_rrs  = 0;
		uint32_t iteration_n_hprs = 0;
		uint32_t iteration_n_errs = 0;
        do {
            received_feedback_somewhere = false;

            // check the active channels for feedback
            for (auto& chan_id : activeChannels) {
                // pull all the currently available feedback from this channel
                while(bool received_on_chan = keeper->getEntry(chan_id).fe->clipProcFeedback.waitNotEmptyOrDoneOrTimeout(m_dataProcessingTime)) {
                    received_feedback_somewhere |= received_on_chan;
                    auto params = keeper->getEntry(chan_id).fe->clipProcFeedback.popData();

                    if (params->trigger_tag >=  0) {
                        channelReceivedTriggersCnt[chan_id] += 1;
                        channelReceivedPacketSize[chan_id] += params->packet_size;
                        channelReceivedNClusters[chan_id]  += params->n_clusters;
                    }
                    else if (params->trigger_tag == PROCESSING_FEEDBACK_TRIGGER_TAG_RR) {
                        channelReceivedRRCnt[chan_id]  += 1;
                        iteration_n_rrs++;
                    }
                    else if (params->trigger_tag == PROCESSING_FEEDBACK_TRIGGER_TAG_Control) {
                        channelReceivedControlCnt[chan_id] += 1;
                        iteration_n_hprs++;
                    }
                    else { // SPDLOG_LOGGER_DEBUG(sdllog, "--> StdDataLoop::execPart2 feedback received an unexpected trigger tag {}", params->trigger_tag);
                        iteration_n_errs++;
                    }
                }
            }
        } while (received_feedback_somewhere);

        // test whether all channels received all triggers
        unsigned channels_w_all_trigs_n = 0; // 
        uint32_t n_all_received_triggers_so_far = 0;
        for (auto &[id, received_triggers] : channelReceivedTriggersCnt) {
            //SPDLOG_LOGGER_DEBUG(sdllog, "--> StdDataLoop::execPart2 : chan {} received {} triggers from {}", id, received_triggers, n_triggersToReceive);
			n_all_received_triggers_so_far += received_triggers;

            if (received_triggers >= n_triggersToReceive) {
                //activeChannels.erase(id); // ok, don't erase a channel - it looks like we receive some random 1-2 triggers here and there
                channels_w_all_trigs_n += 1;
            }
        }
        received_all_triggers = channels_w_all_trigs_n >= keeper->getNumOfEntries();

        // test whether there is still time for this iteration
        time_elapsed =
            std::chrono::duration_cast<std::chrono::microseconds>(Clock::now() - time_start);
        there_is_still_time = time_elapsed.count() < m_totalIterationTime.count();

        // Check if trigger is done
        trigger_is_done = g_tx->isTrigDone();

        // Whether to execute another Rx cycle:
        receiving_rx_data = !trigger_is_done || (there_is_still_time && !received_all_triggers);
        SPDLOG_LOGGER_DEBUG(sdllog, "one more Rx cycle: {} -- still time {} = {} < {} and all trigs = {} (n channels w all trigs = {}, n trigs = {}, n rrs hprs errs = {} {} {})",
            receiving_rx_data, there_is_still_time, time_elapsed.count(), m_totalIterationTime.count(), received_all_triggers,
            channels_w_all_trigs_n, n_all_received_triggers_so_far, iteration_n_rrs, iteration_n_hprs, iteration_n_errs);

        // wait the latency for the Rx read cycle
        if (receiving_rx_data) std::this_thread::sleep_for(m_rxReadDelay);
    }

    // the iteration end marker for the processing & analysis
    // send end-of-iteration empty container with LoopStatus::is_end_of_iteration = true
    LoopStatus loop_status_iteration_end({0}, {LoopStyle::LOOP_STYLE_GLOBAL_FEEDBACK});
    loop_status_iteration_end.is_end_of_iteration = true;
    for (unsigned id=0; id<keeper->getNumOfEntries(); id++) {
        std::unique_ptr<RawDataContainer> c_iter_end = std::make_unique<RawDataContainer>(std::move(loop_status_iteration_end));
        keeper->getEntry(id).fe->clipRawData.pushData(std::move(c_iter_end));
    }

    // report the average channel occupancy data
    for (auto &[id, received_triggers] : channelReceivedTriggersCnt) {
        float av_sizes    = ((float) channelReceivedPacketSize[id]) / ((float) received_triggers);
        float av_clusters = ((float) channelReceivedNClusters[id])  / ((float) received_triggers);
        SPDLOG_LOGGER_DEBUG(sdllog, "channel {} received {} triggers, in packets sizes {} with {} clusters", id, received_triggers, av_sizes, av_clusters);
    }

    m_done = true;
    counter++;
}

void StdDataLoop::loadConfig(const json &config) {

    if (config.contains("total_iteration_time_us"))
        m_totalIterationTime = std::chrono::microseconds(config["total_iteration_time_us"]);

    if (config.contains("rx_read_delay_us"))
        m_rxReadDelay = std::chrono::microseconds(config["rx_read_delay_us"]);

    if (config.contains("n_consecutive_rx_reads_before_processing"))
        m_maxConsecutiveRxReads = config["n_consecutive_rx_reads_before_processing"];

    if (config.contains("average_data_processing_time_us"))
        m_dataProcessingTime = std::chrono::microseconds(config["average_data_processing_time_us"]);

    if (config.contains("triggersLostTolerance"))
        n_triggersLostTolerance = config["triggersLostTolerance"];

    SPDLOG_LOGGER_INFO(sdllog, "Configured StdDataLoop: average_data_processing_time_us: {}", m_dataProcessingTime.count());
}
