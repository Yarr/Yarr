/*
 * Authors: T. Heim <timon.heim@cern.ch>,
 * Date: 2014-Sep-27
 */

#include "StdDataLoop.h"

#include <chrono>
#include <thread>
#include <algorithm>

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
    if (verbose)
        std::cout << __PRETTY_FUNCTION__ << std::endl;
}

void StdDataLoop::end() {
    if (verbose)
        std::cout << __PRETTY_FUNCTION__ << std::endl;

}

void StdDataLoop::execPart1() {
    if (verbose)
        std::cout << __PRETTY_FUNCTION__ << std::endl;
    if (g_tx->getTrigEnable() == 0)
        std::cerr << "### ERROR ### " << __PRETTY_FUNCTION__ << " : Trigger is not enabled, will get stuck here!" << std::endl;

}

void StdDataLoop::execPart2() {
    if (verbose)
        std::cout << __PRETTY_FUNCTION__ << std::endl;
    unsigned count = 0;
    uint32_t done = 0;
    //uint32_t rate = 0;
    uint32_t curCnt = 0;
    unsigned iterations = 0;
    uint32_t startAddr = 0;


    std::vector<RawData*> tmp_storage;
    RawData *newData = NULL;
    while (done == 0) {
        //rate = g_rx->getDataRate();
        curCnt = g_rx->getCurCount();
        done = g_tx->isTrigDone();
        do {
            newData =  g_rx->readData();
            iterations++;
            if (newData != NULL) {
                tmp_storage.push_back(newData);
                count += newData->words;
            }
        } while (newData != NULL);
    }
    // Gather rest of data after timeout
    std::this_thread::sleep_for(std::chrono::microseconds(200));
    do {
        curCnt = g_rx->getCurCount();
        newData =  g_rx->readData();
        iterations++;
        if (newData != NULL) {
            tmp_storage.push_back(newData);
            count += newData->words;
        }
    } while (newData != NULL);
    
    // Merge data
    if (tmp_storage.size() > 0) {
        uint32_t *tBuf = new uint32_t[count];
        unsigned offset = 0;
        for (unsigned i=0; i<tmp_storage.size(); i++) {
            if (i==0)
                startAddr = tmp_storage[i]->adr;
            std::copy(&tmp_storage[i]->buf[0], &tmp_storage[i]->buf[tmp_storage[i]->words], &tBuf[offset]);
            offset += tmp_storage[i]->words;
            delete tmp_storage[i];
        }

        RawData *mergedData = new RawData(startAddr, tBuf, count);
        mergedData->stat = *g_stat;
        storage->pushData(mergedData);
    } else  {
        std::cout << __PRETTY_FUNCTION__ << " --> Found no data!" << std::endl;
        RawData *mergedData = new RawData(0, NULL, 0);
        mergedData->stat = *g_stat;
        storage->pushData(mergedData);
    }
        
    if (verbose)
        std::cout << " --> Received " << count << " words! " << iterations << std::endl;
    m_done = true;
    counter++;
}

void StdDataLoop::connect(ClipBoard<RawData> *clipboard) {
    storage = clipboard;
}
