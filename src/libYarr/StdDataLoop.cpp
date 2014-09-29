/*
 * Authors: T. Heim <timon.heim@cern.ch>,
 * Date: 2014-Sep-27
 */

#include "StdDataLoop.h"

StdDataLoop::StdDataLoop() : LoopActionBase() {
    storage = NULL;
}

void StdDataLoop::init() {
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

}

void StdDataLoop::execPart2() {
    if (verbose)
        std::cout << __PRETTY_FUNCTION__ << std::endl;
    uint32_t done = 0;
    RawData *newData = NULL;
    while (done == 0 || newData == NULL) {
        done = g_tx->isTrigDone();
        newData =  g_rx->readData();
        if (newData != NULL) {
            storage->pushData(newData);
        }
    }
    m_done = true;
}

void StdDataLoop::connect(ClipBoard<RawData> *clipboard) {
    storage = clipboard;
}

