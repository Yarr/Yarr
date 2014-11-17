// #################################
// # Author: Timon Heim
// # Email: timon.heim at cern.ch
// # Project: Yarr
// # Description: Bookkeeper
// # Comment: Global container for data
// ################################

#include "Bookkeeper.h"

Bookkeeper::Bookkeeper() {

}

// Delete all leftover data, Bookkeeper should be deleted last
Bookkeeper::~Bookkeeper() {
    // Raw Data
    rawDataMutex.lock();
    for(std::deque<RawData*>::iterator it = rawDataList.begin(); it != rawDataList.end(); ++it)
        delete (*it);
    rawDataMutex.unlock();

    // Processed Data
    procDataMutex.lock();
    for(std::deque<Fei4Data*>::iterator it = procDataList.begin(); it != procDataList.end(); ++it)
        delete (*it);
    procDataMutex.unlock();

    // Histogrammer Data
    histoMutex.lock();
    for(std::deque<HistogramBase*>::iterator it = histoList.begin(); it != histoList.end(); ++it)
        delete (*it);
    histoMutex.unlock();
    
    // Analyzed Data
    resultMutex.lock();
    for(std::deque<ResultBase*>::iterator it = resultList.begin(); it != resultList.end(); ++it)
        delete (*it);
    resultMutex.unlock();
}

void Bookkeeper::pushData(RawData *d) {
    rawDataMutex.lock();
    rawDataList.push_back(d);
    rawDataMutex.unlock();
}

void Bookkeeper::pushData(Fei4Data *d) {
    procDataMutex.lock();
    procDataList.push_back(d);
    procDataMutex.unlock();
}

void Bookkeeper::pushData(HistogramBase *h) {
    histoMutex.lock();
    histoList.push_back(h);
    histoMutex.unlock();
}

void Bookkeeper::pushData(ResultBase *r) {
    resultMutex.lock();
    resultList.push_back(r);
    resultMutex.unlock();
}

void Bookkeeper::popData(RawData *d) {
    rawDataMutex.lock();
    d = rawDataList.front();
    rawDataList.pop_front();
    rawDataMutex.unlock();
}

void Bookkeeper::popData(Fei4Data *d) {
    procDataMutex.lock();
    d = procDataList.front();
    procDataList.pop_front();
    procDataMutex.unlock();
}

void Bookkeeper::popData(HistogramBase *h) {
    histoMutex.lock();
    h = histoList.front();
    histoList.pop_front();
    histoMutex.unlock();
}

void Bookkeeper::popData(ResultBase *r) {
    resultMutex.lock();
    r = resultList.front();
    resultList.pop_front();
    resultMutex.unlock();
}

