#ifndef YARR_HISTOGRAM_PROCESSOR_H
#define YARR_HISTOGRAM_PROCESSOR_H

#include <memory>

#include "ClipBoard.h"
#include "HistoDataProcessor.h"
#include "HistogramBase.h"

/**
 * Process a stream of events using the registered HistogramAlgorithms.
 */
class HistogrammerProcessor : public HistoDataProcessor {
    public:
        HistogrammerProcessor();
        virtual ~HistogrammerProcessor() override;

        void connect(ClipBoard<EventDataBase> *arg_input, ClipBoard<HistogramBase> *arg_output) override {
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

        void init() override;
        void run() override;
        void join() override;
        void process() override;
        void process_core();
        void publish();

    private:
        ClipBoard<EventDataBase> *input = nullptr;
        ClipBoard<HistogramBase> *output = nullptr;
        std::unique_ptr<std::thread> thread_ptr;

        std::vector<std::unique_ptr<HistogramAlgorithm>> algorithms;
        bool is_new_iteration = true;
};

#endif
