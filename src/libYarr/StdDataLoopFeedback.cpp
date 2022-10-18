/*
 * Authors: A. Toldaiev <alex.toldaiev@cern.ch>,
 * Date: 2022-Sep-5
 */

#include "StdDataLoopFeedback.h"

#include <chrono>
using Clock = std::chrono::steady_clock;

#include "logging.h"

namespace {
    auto sdllog = logging::make_log("StdDataLoopFeedback");
}

void StdDataLoopFeedback::execPart2() {
    SPDLOG_LOGGER_TRACE(sdllog, "");
    unsigned iterations = 0;

    // all RX channels in the scan
    std::map<uint32_t, uint32_t> channelReceivedTriggersCnt;
    std::map<uint32_t, uint32_t> channelReceivedRRCnt;
    std::map<uint32_t, uint32_t> channelReceivedHPRCnt;

    for (unsigned id=0; id<keeper->getNumOfEntries(); id++) {
        channelReceivedTriggersCnt[id] = 0;
        channelReceivedRRCnt[id]       = 0;
        channelReceivedHPRCnt[id]      = 0;
    }

    // the RX channels that actually received data, and expect feedback
    std::map<uint32_t, uint32_t> activeChannels;

    bool received_all_triggers = true;
    bool still_time = false;
    uint32_t n_triggers_to_receive = 500; // g_tx->getTrigCnt(); // TODO find this number
    std::chrono::microseconds time_elapsed;
    std::chrono::microseconds wait_time = 1000 * 10 * g_rx->getWaitTime() * 10; // emulator is slow x10
    std::chrono::microseconds wait_time_2 = 50 * g_rx->getWaitTime();
    std::chrono::time_point<Clock> time_start = Clock::now();

    do {
        unsigned count = 0;
        unsigned count_chunks = 0;
        uint32_t done = 0;

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
                        count += dataChunk->getSize();
                        for (unsigned &uid : keeper->getRxToId(dataChunk->getAdr())) {
                            if (rdcMap[uid] == nullptr) {
                                rdcMap[uid] = std::make_unique<RawDataContainer>(g_stat->record());
                                // init active channels with data
                                //channelReceivedTriggersCnt[uid] += 0; // TODO make this logic nicer
                                //channelReceivedRRCnt[uid]       += 0;
                                //channelReceivedHPRCnt[uid]      += 0;
                                activeChannels[uid] += 0;
                            }

                            rdcMap[uid]->add(dataChunk);
                            count_chunks += 1;
                        }
                    }
                }
            } while (newData.size() > 0);
        }

        std::chrono::time_point<Clock> time_current = Clock::now();
        time_elapsed =
            std::chrono::duration_cast<std::chrono::microseconds>(time_current - time_start);
        SPDLOG_LOGGER_DEBUG(sdllog, "--> StdDataLoopFeedback::execPart2 : time_elapsed after first RX = {} (wait time {})", time_elapsed.count(), wait_time.count());

        // Gather rest of data after timeout (defined by controller)
        //std::this_thread::sleep_for(g_rx->getWaitTime());
        //SPDLOG_LOGGER_DEBUG(sdllog, "!!! waiting {}", wait_time.count());

        // Gather the data until the number of issued triggers is received
        // or a time limit passes
        std::this_thread::sleep_for(g_rx->getWaitTime());

        do { // read all currently received RX data

            //curCnt = g_rx->getCurCount();
            newData = g_rx->readData();
            iterations++;
            if (newData.size() > 0) {
                for (auto &dataChunk : newData) {
                    count += dataChunk->getSize();
                    for (unsigned &uid : keeper->getRxToId(dataChunk->getAdr())) {
                        if (rdcMap[uid] == nullptr) {
                            rdcMap[uid] = std::make_unique<RawDataContainer>(g_stat->record());
                            // init active channels with data
                            //channelReceivedTriggersCnt[uid] += 0; // TODO make this logic nicer
                            //channelReceivedRRCnt[uid]       += 0;
                            //channelReceivedHPRCnt[uid]      += 0;
                            activeChannels[uid] += 0;
                        }

                        rdcMap[uid]->add(dataChunk);
                        count_chunks += 1;
                    }
                }
            }
        } while (newData.size() > 0 || g_rx->getCurCount() != 0);

        time_current = Clock::now();
        time_elapsed =
            std::chrono::duration_cast<std::chrono::microseconds>(time_current - time_start);
        SPDLOG_LOGGER_DEBUG(sdllog, "--> StdDataLoopFeedback::execPart2 : time_elapsed after second RX = {} (wait time {})", time_elapsed.count(), wait_time.count());

        // push data for processing
        for (auto &[id, rdc] : rdcMap) {
            SPDLOG_LOGGER_DEBUG(sdllog, "\033[1m\033[31m pushData for processing {} {}\033[0m", id, rdc->size());
            rdc->stat.is_end_of_iteration = false;
            keeper->getEntry(id).fe->clipRawData.pushData(std::move(rdc));
        }

        if (count == 0) {
        SPDLOG_LOGGER_DEBUG(sdllog, "\033[1m\033[31m--> Received {} words in {} iterations!\033[0m", count ,iterations);
        //break;
        std::this_thread::sleep_for(wait_time_2);
        } else {
        SPDLOG_LOGGER_DEBUG(sdllog, "--> Received {} words in {} iterations! in {} RawData", count, iterations, count_chunks);
        }

        // check how many events went through the data processors
        bool received_feedback = false;
        do {
            received_feedback = false;
            std::this_thread::sleep_for(g_rx->getWaitTime());

            // loop over all active data channels, not just the ones from the last push
            //for (const auto &item : channelReceivedTriggersCnt)
            for (const auto &item : activeChannels) { // TODO: filter channels that received all triggers
                uint32_t channel_n = item.first;

                //SPDLOG_LOGGER_DEBUG(sdllog, "--> StdDataLoopFeedback::execPart2 to receive feedback on channel {}", channel_n);
                bool received_on_chan = feedbackFromRawDataProcessing->at(channel_n).waitNotEmptyOrDoneOrTimeout(std::chrono::microseconds(100));
                received_feedback |= received_on_chan;

                if (!received_on_chan) continue;

                // params is a unique_ptr of FeedbackProcessingInfo
                auto params = feedbackFromRawDataProcessing->at(channel_n).popData();
                //SPDLOG_LOGGER_DEBUG(sdllog, "--> StdDataLoopFeedback::execPart2 feedback : channel {} trigger tag {}", channel_n, params->trigger_tag);

                // sum up the processed events for each channel
                if (params->trigger_tag >=  0) channelReceivedTriggersCnt[channel_n] += 1;
                else if (params->trigger_tag == -2) channelReceivedRRCnt[channel_n]  += 1;
                else if (params->trigger_tag == -3) channelReceivedHPRCnt[channel_n] += 1;
                else {
                    SPDLOG_LOGGER_DEBUG(sdllog, "--> StdDataLoopFeedback::execPart2 feedback received an unexpected trigger tag {}", params->trigger_tag);
                }
            }
        } while (received_feedback); // add time out here too?

        // test whether all channels received all expected triggers
        received_all_triggers = true;
        for (auto &[id, received_triggers] : channelReceivedTriggersCnt) {
            SPDLOG_LOGGER_DEBUG(sdllog, "--> StdDataLoopFeedback::execPart2 : chan {} received {} triggers from {}", id, received_triggers, n_triggers_to_receive);
            received_all_triggers &= received_triggers >= n_triggers_to_receive;
            //if (received_all_triggers) break;
        }

        time_current = Clock::now();
        time_elapsed =
            std::chrono::duration_cast<std::chrono::microseconds>(time_current - time_start);
        still_time = time_elapsed.count() < wait_time.count();
        SPDLOG_LOGGER_DEBUG(sdllog, "--> StdDataLoopFeedback::execPart2 : still_time {} = {} < {}", still_time, time_elapsed.count(), wait_time.count());

    } while (!received_all_triggers && still_time);

    // send end-of-iteration empty container with LoopStatus::is_end_of_iteration = true
    LoopStatus loop_status_iteration_end({0}, {LoopStyle::LOOP_STYLE_GLOBAL_FEEDBACK});
    loop_status_iteration_end.is_end_of_iteration = true;
    for (unsigned id=0; id<keeper->getNumOfEntries(); id++) {
        std::unique_ptr<RawDataContainer> c_iter_end = std::make_unique<RawDataContainer>(std::move(loop_status_iteration_end));
        keeper->getEntry(id).fe->clipRawData.pushData(std::move(c_iter_end));
    }

    for (auto &[id, received_triggers] : channelReceivedTriggersCnt) {
        SPDLOG_LOGGER_DEBUG(sdllog, "--> StdDataLoopFeedback::execPart2 FINISH : chan {} received {} triggers from {}", id, received_triggers, n_triggers_to_receive);
    }

    SPDLOG_LOGGER_DEBUG(sdllog, "--> StdDataLoopFeedback::execPart2 FINISH : received all triggers = {}, elapsed time {} from delay limit {} => there is still time = {}", received_all_triggers, time_elapsed.count(), wait_time.count(), time_elapsed.count() < wait_time.count());

    m_done = true;
    //counter++;
}
