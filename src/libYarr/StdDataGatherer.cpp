/*
 * Authors: T. Heim <timon.heim@cern.ch>,
 * Date: 2015-Jul-21
 */

#include "StdDataGatherer.h"

#include <chrono>
#include <iostream>
#include <thread>
#include <algorithm>

#include <signal.h>

#include "logging.h"

namespace {
    auto sdglog = logging::make_log("StdDataGatherer");
}

StdDataGatherer::StdDataGatherer() : LoopActionBase(LOOP_STYLE_DATA) {
    storage = NULL;
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
    bool done = false;

    signaled = 0;
    signal(SIGINT, [](int signum){signaled = 1;});
    signal(SIGUSR1, [](int signum){signaled = 1;});

    SPDLOG_LOGGER_WARN(sdglog, "IMPORTANT! Going into endless loop unless timelimit is set, interrupt with ^c (SIGINT)!");

    std::vector<std::shared_ptr<RawData>> newData;
    while (!done) {
        std::map<uint32_t, std::unique_ptr<RawDataContainer>> rdcMap;
        
        done = g_tx->isTrigDone();
        newData =  g_rx->readData();
        
        // Read all data until buffer is empty
        while (newData.size() > 0 && count < 4096 && signaled == 0 && !killswitch) {
            if (newData.size() > 0) {
                for (auto &dataChunk : newData) {
                    count += dataChunk->getSize();
                    if (rdcMap[dataChunk->getAdr()] == nullptr) {
                        rdcMap[dataChunk->getAdr()] = std::make_unique<RawDataContainer>(g_stat->record());
                    }

                    rdcMap[dataChunk->getAdr()]->add(std::move(dataChunk));
                }
            }
            // Wait a little bit to increase chance of new data having arrived
            std::this_thread::sleep_for(std::chrono::microseconds(1));
            newData =  g_rx->readData();
        }

        for (auto &[id, rdc] : rdcMap) {
            storage->at(id).pushData(std::move(rdc));
        }
        
        count = 0;

        if (signaled == 1 || killswitch) {
            SPDLOG_LOGGER_WARN(sdglog, "Caught interrupt, stopping data taking!");
            SPDLOG_LOGGER_WARN(sdglog, "Abort might leave data in buffers!");
            g_tx->toggleTrigAbort();
        }
        std::this_thread::sleep_for(g_rx->getWaitTime());
    }

    m_done = true;
    counter++;
}

//void StdDataGatherer::connect(ClipBoard<RawDataContainer> *clipboard) {
//    storage = clipboard;
//}
