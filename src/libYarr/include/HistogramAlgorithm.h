#ifndef YARR_HISTOGRAM_ALGORITHM_H
#define YARR_HISTOGRAM_ALGORITHM_H

#include <memory>
#include <thread>

#include "HistogramBase.h"
#include "LoopStatus.h"

// Could be EventDataBase?
class FrontEndData;

/**
 * Process a stream of events and produce a histogram.
 */
class HistogramAlgorithm {
    public:
        HistogramAlgorithm() {
            nCol = 80;
            nRow = 336;
        
        }
        virtual ~HistogramAlgorithm() = default;

        virtual void create(const LoopStatus &stat) {}
        
        std::unique_ptr<HistogramBase> getHisto() {
            return std::move(r);
        }
        
        virtual void processEvent(FrontEndData *data) {}
        virtual void loadConfig(const json &config) {}
        
        void setMapSize(unsigned col, unsigned row) {
            nCol = col;
            nRow = row;
        }
    protected:
        std::unique_ptr<HistogramBase> r;
        unsigned nCol = 80;
        unsigned nRow = 336;
};

#endif
