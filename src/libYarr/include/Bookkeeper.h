#ifndef BOOKKEEPER_H
#define BOOKKEEPER_H

// #################################
// # Author: Timon Heim
// # Email: timon.heim at cern.ch
// # Project: Yarr
// # Description: Bookkeeper
// # Comment: Global container for data
// ################################

#include <iostream>
#include <mutex>
#include <deque>

#include "RawData.h"
#include "Fei4EventData.h"
#include "HistogramBase.h"
#include "ResultBase.h"

class Bookkeeper {
    public:
        Bookkeeper();
        ~Bookkeeper();

        void pushData(RawData *d);
        void pushData(Fei4Data *d);
        void pushData(HistogramBase *h);
        void pushData(ResultBase *r);

        void popData(RawData *d);
        void popData(Fei4Data *d);
        void popData(HistogramBase *h);
        void popData(ResultBase *r);

    private:

        // Raw Data
        std::deque<RawData*> rawDataList;
        std::mutex rawDataMutex;

        // Processed Data
        std::deque<Fei4Data*> procDataList;
        std::mutex procDataMutex;

        // Histogrammer Data
        std::deque<HistogramBase*> histoList;
        std::mutex histoMutex;

        // Analyzed Data
        std::deque<ResultBase*> resultList;
        std::mutex resultMutex;

};

#endif
