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

StdDataLoop::StdDataLoop() : LoopActionBase() {
    storage = NULL;
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
    //uint32_t rate = 0;
    //uint32_t curCnt = 0;
    unsigned iterations = 0;
    //uint32_t startAddr = 0;


    std::vector<RawData*> tmp_storage;
    RawData *newData = NULL;
    std::unique_ptr<RawDataContainer> rdc(new RawDataContainer(g_stat->record()));
    while (done == 0) {
        //rate = g_rx->getDataRate();
        //curCnt = g_rx->getCurCount();
        done = g_tx->isTrigDone();
        do {
            newData =  g_rx->readData();
            iterations++;
            if (newData != NULL) {
                count += newData->words;
                rdc->add(newData);
            }
        } while (newData != NULL);
        //delete newData;
    }
    // Gather rest of data after timeout (defined by controller)
    std::this_thread::sleep_for(g_rx->getWaitTime());
    do {
        //curCnt = g_rx->getCurCount();
        newData = g_rx->readData();
        iterations++;
        if (newData != NULL) {
            count += newData->words;
            rdc->add(newData);
        }
    } while (newData != NULL || g_rx->getCurCount() != 0);
    delete newData;
    
    storage->pushData(std::move(rdc));
        
    SPDLOG_LOGGER_DEBUG(sdllog, "--> Received {} words in {} iterations!", count ,iterations);
    m_done = true;
    counter++;
}

//void StdDataLoop::connect(ClipBoard<RawDataContainer> *clipboard) {
//    storage = clipboard;
//}
