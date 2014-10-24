/*
 * Authors: T. Heim <timon.heim@cern.ch>,
 * Date: 2014-Sep-27
 */

#include "StdDataLoop.h"
#include <unistd.h>

StdDataLoop::StdDataLoop() : LoopActionBase() {
    storage = NULL;
    loopType = typeid(this);
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
    //uint32_t rate = -1;
    unsigned iterations = 0;
    RawData *newData = NULL;
    while (done == 0) {
        //rate = g_rx->getDataRate();
        done = g_tx->isTrigDone();
        do {
            newData =  g_rx->readData();
            iterations++;
            if (newData != NULL) {
                newData->stat = *g_stat;
                storage->pushData(newData);
                count += newData->words;
            }
        } while (newData != NULL);
    }
    usleep(50);
    do {
        newData =  g_rx->readData();
        iterations++;
        if (newData != NULL) {
            newData->stat = *g_stat;
            storage->pushData(newData);
            count += newData->words;
        }
    } while (newData != NULL);
    
    if (verbose)
        std::cout << " --> Received " << count << " words! " << iterations << std::endl;
    m_done = true;
}

void StdDataLoop::connect(ClipBoard<RawData> *clipboard) {
    storage = clipboard;
}

