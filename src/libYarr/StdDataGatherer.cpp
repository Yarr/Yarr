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

#include "Bookkeeper.h"
#include "RxCore.h"
#include "TxCore.h"

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
    m_passData = true;
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
    std::this_thread::sleep_for(g_rx->getWaitTime());

    SPDLOG_LOGGER_WARN(sdglog, "IMPORTANT! Going into endless loop unless timelimit is set, interrupt with ^c (SIGINT)!");

    bool receivingRxData = true;

    if(m_passData) {

        std::vector<RawDataPtr> newData;
        std::map<uint32_t, std::unique_ptr<RawDataContainer>> rdcMap;

        while (receivingRxData) {
            // Whether to execute another Rx cycle:
            receivingRxData = !g_tx->isTrigDone();

            newData =  g_rx->readData();
            nAllRxReadIterations++;

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
                newData.clear();
            } else {
                // Wait a little bit to increase chance of new data having arrived
                std::this_thread::sleep_for(std::chrono::microseconds(10));
            }

            // Accumulate data either until max number of chunks or chunks larger than max size
            if (count > m_maxRxReadSize || nAllRxReadIterations > m_maxConsecutiveRxReads) {

                // Push the accumulated chunks for processing
                for (auto &[id, rdc] : rdcMap) {
                    if (rdc->size() > 0) { // Only push when not empty
                        // Push data out
                        rdc->stat.is_end_of_iteration = false;
                        keeper->getFe(id)->clipRawData.pushData(std::move(rdc));
                        // Create EoI
                        LoopStatus loopStatusIterationEnd({0}, {LoopStyle::LOOP_STYLE_GLOBAL_FEEDBACK});
                        loopStatusIterationEnd.is_end_of_iteration = true;
                        // Send EoI
                        std::unique_ptr<RawDataContainer> cIterEnd = std::make_unique<RawDataContainer>(std::move(loopStatusIterationEnd));
                        keeper->getFe(id)->clipRawData.pushData(std::move(cIterEnd));
                        keeper->getFe(id)->clipProcFeedback.clearData();
                    }
                }
                rdcMap.clear();
                SPDLOG_LOGGER_DEBUG(sdglog, "--> Received {} words in {} iterations!", count, nAllRxReadIterations);
                count = 0;
                nAllRxReadIterations = 0;
            }

            if ((signaled == 1 || killswitch) && receivingRxData) {
                SPDLOG_LOGGER_WARN(sdglog, "Caught interrupt, stopping data taking!");
                SPDLOG_LOGGER_WARN(sdglog, "Abort might leave data in buffers!");
                g_tx->toggleTrigAbort();
            }
        }

        // Process leftover chunks
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
        newData.clear();

        // Push any remaining data for processing
        for (auto &[id, rdc] : rdcMap) {
            if (rdc->size() > 0) {
                rdc->stat.is_end_of_iteration = false;
                keeper->getFe(id)->clipRawData.pushData(std::move(rdc));
            }
        }
        rdcMap.clear();

        // Send last EoI
        for (unsigned id=0; id<keeper->getNumOfEntries(); id++) {
            LoopStatus loopStatusIterationEnd({0}, {LoopStyle::LOOP_STYLE_GLOBAL_FEEDBACK});
            loopStatusIterationEnd.is_end_of_iteration = true;
            std::unique_ptr<RawDataContainer> cIterEnd = std::make_unique<RawDataContainer>(std::move(loopStatusIterationEnd));
            keeper->getFe(id)->clipRawData.pushData(std::move(cIterEnd));
            keeper->getFe(id)->clipProcFeedback.clearData();
        }
    }
    else {
        SPDLOG_LOGGER_WARN(sdglog, "NOT PROCESSING ANY DATA", m_maxConsecutiveRxReads);      
        while (receivingRxData) {
            std::vector<RawDataPtr> newData;
            newData =  g_rx->readData();
            newData.clear();
            if (signaled == 1 || killswitch) {
                SPDLOG_LOGGER_WARN(sdglog, "Caught interrupt, stopping data taking!");
                SPDLOG_LOGGER_WARN(sdglog, "Abort might leave data in buffers!");
                g_tx->toggleTrigAbort();
            }
            receivingRxData = !g_tx->isTrigDone();
        }
    }

    m_done = true;
    counter++;
}

void StdDataGatherer::loadConfig(const json &config) {
    if (config.contains("maxConsecutiveRxReads")) {
        m_maxConsecutiveRxReads = config["maxConsecutiveRxReads"];
        SPDLOG_LOGGER_INFO(sdglog, "Configured StdDataGatherer: maxConsecutiveRxReads: {} [times]", m_maxConsecutiveRxReads);
    }
    if (config.contains("maxRxReadSize")) {
        m_maxRxReadSize = config["maxRxReadSize"];
        SPDLOG_LOGGER_INFO(sdglog, "Configured StdDataGatherer: maxRxReadSize: {} [words]", m_maxConsecutiveRxReads);
    }
    if (config.contains("passData")) {
        m_passData = config["passData"];
        SPDLOG_LOGGER_INFO(sdglog, "Configured StdDataGatherer: passData: {}", m_passData);
    }
    else
        m_passData = true;
}
