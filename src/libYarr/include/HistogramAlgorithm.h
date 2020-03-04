#ifndef YARR_HISTOGRAM_ALGORITHM_H
#define YARR_HISTOGRAM_ALGORITHM_H

#include <memory>

#include "HistogramBase.h"
#include "LoopStatus.h"

// Could be EventDataBase?
class Fei4Data;

/**
 * Process a stream of events and produce a histogram.
 */
class HistogramAlgorithm {
    public:
        HistogramAlgorithm() {
            nCol = 80;
            nRow = 336;
        
        }
        virtual ~HistogramAlgorithm() {}

        virtual void create(LoopStatus &stat) {}
        
        std::unique_ptr<HistogramBase> getHisto() {
            return std::move(r);
        }
        
        virtual void processEvent(Fei4Data *data) {}
        void setMapSize(unsigned col, unsigned row) {
            nCol = col;
            nRow = row;
        }
    protected:
        std::unique_ptr<HistogramBase> r;
        unsigned nCol;
        unsigned nRow;
};

#endif
