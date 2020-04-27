#ifndef YARR_HISTOGRAM_ALGORITHM_H
#define YARR_HISTOGRAM_ALGORITHM_H

#include <memory>
#include <thread>

#include "DataProcessor.h"
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

/**
 * Process a stream of events using registered HistogramAlgorithm.
 */
class HistogrammerProcessor : public DataProcessor {
    public:
        HistogrammerProcessor();
        ~HistogrammerProcessor();

        void connect(ClipBoard<EventDataBase> *arg_input, ClipBoard<HistogramBase> *arg_output) {
            input = arg_input;
            output = arg_output;
        }

        void addHistogrammer(std::unique_ptr<HistogramAlgorithm> a) {
            algorithms.push_back(std::move(a));
        }

        void setMapSize(unsigned col, unsigned row) {
            for (unsigned i=0; i<algorithms.size(); i++) {
                algorithms[i]->setMapSize(col, row);
            }
        }
        
        void clearHistogrammers();

        void init();
        void run();
        void join();
        void process();
        void process_core();
        void publish();

        ClipBoard<EventDataBase>& getInput() { return *input; }

    private:
        ClipBoard<EventDataBase> *input;
        ClipBoard<HistogramBase> *output;
        std::unique_ptr<std::thread> thread_ptr;

        std::vector<std::unique_ptr<HistogramAlgorithm>> algorithms;
};

#endif
