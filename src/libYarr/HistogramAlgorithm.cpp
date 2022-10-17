// #################################
// # Author: Timon Heim
// # Email: timon.heim at cern.ch
// # Project: Yarr
// # Description: Histograms event data
// # Comment:  Moved to libYarr by Bruce Gallop
// ################################

#include "HistogramAlgorithm.h"

#include "EventData.h"

#include "logging.h"

namespace {
    auto alog = logging::make_log("HistogramAlgorithm");
}

HistogrammerProcessor::HistogrammerProcessor() = default;

HistogrammerProcessor::~HistogrammerProcessor() = default;

void HistogrammerProcessor::init() {
}

void HistogrammerProcessor::clearHistogrammers() {
    algorithms.clear();
}


void HistogrammerProcessor::run() {
    thread_ptr = std::make_unique<std::thread>( &HistogrammerProcessor::process, this );
}

void HistogrammerProcessor::join() {
    if( thread_ptr->joinable() ) thread_ptr->join();
}

void HistogrammerProcessor::process() {
    while( true ) {
        input->waitNotEmptyOrDone();

        std::this_thread::sleep_for(std::chrono::milliseconds(200));
        process_core();

        if( input->isDone() ) {
            std::this_thread::sleep_for(std::chrono::milliseconds(200));
            process_core();  // this line is needed if the data comes in before scanDone is changed.
            alog->info("Histogrammer done!");
            break;
        }
    }

    process_core();
}

void HistogrammerProcessor::process_core() {
    while (!input->empty()) {
        auto d = input->popData();
        FrontEndData *data = dynamic_cast<FrontEndData*>(d.get());
        if (data == nullptr)
            continue;

        // process the data
        for (unsigned i=0; i<algorithms.size(); i++) {
            // create on new iteration
            if (is_new_iteration) {
                algorithms[i]->create(data->lStat);
            }
            algorithms[i]->processEvent(data);
        }

        // set an iteration is ongoing
        if (is_new_iteration) { is_new_iteration = false; }

        // if it is the marker of the end of scan iteration:
        // publish, reset the iteration
        if (data->lStat.is_end_of_iteration) {
            this->publish();
            is_new_iteration = true; // reset for the next iteration
        }
    }
}

void HistogrammerProcessor::publish() {
    for (unsigned i=0; i<algorithms.size(); i++) {
        auto ptr = algorithms[i]->getHisto();
        if(ptr) {
            output->pushData(std::move(ptr));
        }
    }
}
