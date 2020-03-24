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

StdDataGatherer::StdDataGatherer() : LoopActionBase() {
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
    uint32_t done = 0;
    uint32_t rate = 0;

    signaled = 0;
    signal(SIGINT, [](int signum){signaled = 1;});
    signal(SIGUSR1, [](int signum){signaled = 1;});

    SPDLOG_LOGGER_WARN(sdglog, "IMPORTANT! Going into endless loop unless timelimit is set, interrupt with ^c (SIGINT)!");

    std::vector<RawData*> tmp_storage;
    RawData *newData = NULL;
    while (done == 0) {
        std::unique_ptr<RawDataContainer> rdc(new RawDataContainer(g_stat->record()));
        rate = g_rx->getDataRate();
        SPDLOG_LOGGER_DEBUG(sdglog, " --> Data Rate: {} MB/s", rate/256.0/1024.0);
        done = g_tx->isTrigDone();
        do {
            newData =  g_rx->readData();
            if (newData != NULL) {
                rdc->add(newData);
                count += newData->words;
                newData = NULL;
            }
            std::this_thread::sleep_for(std::chrono::microseconds(100));
        } while (newData != NULL && signaled == 0 && !killswitch );
        if (newData != NULL)
            delete newData;
        storage->pushData(std::move(rdc));
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
