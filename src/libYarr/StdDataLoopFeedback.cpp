/*
 * Authors: A. Toldaiev <alex.toldaiev@cern.ch>,
 * Date: 2022-Sep-5
 */

#include "StdDataLoopFeedback.h"

//#include <iostream>
//#include <thread>
//#include <algorithm>

#include <chrono>
using Clock = std::chrono::steady_clock;

#include "logging.h"

namespace {
    auto sdllog = logging::make_log("StdDataLoopFeedback");
}

void StdDataLoopFeedback::dataproc_feedback(unsigned channel, struct DataProcFeedbackParams p)
{
    SPDLOG_LOGGER_DEBUG(sdllog, "--> StdDataLoop::dataproc_feedback channel {} event counter {} trigger tag {}", channel, p.event_count, p.trigger_tag);
}

void StdDataLoopFeedback::execPart2() {
    SPDLOG_LOGGER_TRACE(sdllog, "");
    unsigned count = 0;
    uint32_t done = 0;
    unsigned iterations = 0;


    std::vector<RawDataPtr> newData;
    std::map<uint32_t, std::unique_ptr<RawDataContainer>> rdcMap;
    std::map<uint32_t, uint32_t> channelReceivedTriggersCnt;

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
                        }

                        rdcMap[uid]->add(dataChunk);
                    }
                }
            }
        } while (newData.size() > 0);
    }

    // Gather rest of data after timeout (defined by controller)
    //std::this_thread::sleep_for(g_rx->getWaitTime());
    std::chrono::microseconds wait_time = g_rx->getWaitTime();
    //SPDLOG_LOGGER_DEBUG(sdllog, "!!! waiting {}", wait_time.count());
    //std::this_thread::sleep_for(wait_time);

    // Gather the data until the number of issued triggers is received
    // or a time limit passes
    uint32_t n_triggers_to_receive = g_tx->getTrigCnt();
    bool received_all_triggers = true;
    bool still_time = false;
    std::chrono::time_point<Clock> time_start = Clock::now();
    std::chrono::microseconds time_elapsed;

    do {
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
                        }

                        rdcMap[uid]->add(dataChunk);
                    }
                }
            }
        } while (newData.size() > 0 || g_rx->getCurCount() != 0);

        // push data for processing
        for (auto &[id, rdc] : rdcMap) {
            keeper->getEntry(id).fe->clipRawData.pushData(std::move(rdc));
        }

        if (count == 0) {
        SPDLOG_LOGGER_DEBUG(sdllog, "\033[1m\033[31m--> Received {} words in {} iterations!\033[0m", count ,iterations);
        } else {
        SPDLOG_LOGGER_DEBUG(sdllog, "--> Received {} words in {} iterations!", count ,iterations);
        }

        // check how many events went through the data processors
        for (const auto &item : rdcMap) {
            uint32_t channel_n = item.first;

            struct DataProcFeedbackParams params = waitForDataProcFeedback(channel_n);
            SPDLOG_LOGGER_DEBUG(sdllog, "--> StdDataLoopFeedback::execPart2 feedback : channel {} event counter {} trigger tag {}", channel_n, params.event_count, params.trigger_tag);

            // sum up the processed events for each channel
            channelReceivedTriggersCnt[channel_n] += params.event_count;
        }

        // test whether all channels received all expected triggers
        for (auto &[id, received_triggers] : channelReceivedTriggersCnt) {
            received_all_triggers = received_triggers >= n_triggers_to_receive;
            if (!received_all_triggers) break;
        }

        std::chrono::time_point<Clock> time_current = Clock::now();
        time_elapsed =
            std::chrono::duration_cast<std::chrono::microseconds>(time_current - time_start);
        still_time = time_elapsed.count() < wait_time.count();

    } while (!received_all_triggers && still_time);

    SPDLOG_LOGGER_DEBUG(sdllog, "--> StdDataLoopFeedback::execPart2 feedback FINISH : received all triggers {} elapsed time {} from delay limit {}", received_all_triggers, time_elapsed.count(), wait_time.count());

    m_done = true;
    counter++;
}
