/*
 * Authors: T. Heim <timon.heim@cern.ch>,
 * Date: 2014-Sep-27
 */

#include "StdDataLoop.h"

#include <chrono>
#include <iostream>
#include <thread>
#include <algorithm>

#include "logging.h"

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
    unsigned count = 0;
    uint32_t done = 0;
    unsigned iterations = 0;


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
                        }

                        rdcMap[uid]->add(dataChunk);
                    }
                }
            }
        } while (newData.size() > 0);
    }

    // Gather rest of data after timeout (defined by controller)
    std::this_thread::sleep_for(g_rx->getWaitTime());
    do {
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
    
    for (auto &[id, rdc] : rdcMap) {
        keeper->getEntry(id).fe->clipRawData.pushData(std::move(rdc));
    }

    // push the empty container that marks the end of the scan iteration
    // for the processors, histogrammers, analysis, etc threads
    LoopStatus loop_status_iteration_end({0}, {LoopStyle::LOOP_STYLE_GLOBAL_FEEDBACK});
    loop_status_iteration_end.is_end_of_iteration = true;
    for (unsigned id=0; id<keeper->getNumOfEntries(); id++) {
        std::unique_ptr<RawDataContainer> c_iter_end = std::make_unique<RawDataContainer>(std::move(loop_status_iteration_end));
        keeper->getEntry(id).fe->clipRawData.pushData(std::move(c_iter_end));
    }

    if (count == 0) {
      SPDLOG_LOGGER_DEBUG(sdllog, "\033[1m\033[31m--> Received {} words in {} iterations!\033[0m", count ,iterations);
    } else {
      SPDLOG_LOGGER_DEBUG(sdllog, "--> Received {} words in {} iterations!", count ,iterations);
    }
    m_done = true;
    counter++;
}
