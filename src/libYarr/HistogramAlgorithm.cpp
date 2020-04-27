// #################################
// # Author: Timon Heim
// # Email: timon.heim at cern.ch
// # Project: Yarr
// # Description: Histograms event data
// # Comment:  Moved to libYarr by Bruce Gallop
// ################################

#include "HistogramAlgorithm.h"

#include "Fei4EventData.h"

#include "logging.h"

namespace {
    auto alog = logging::make_log("HistogramAlgorithm");
}

HistogrammerProcessor::HistogrammerProcessor() {
}

HistogrammerProcessor::~HistogrammerProcessor() {
}

void HistogrammerProcessor::init() {
}

void HistogrammerProcessor::clearHistogrammers() {
    algorithms.clear();
}


void HistogrammerProcessor::run() {
    thread_ptr.reset( new std::thread( &HistogrammerProcessor::process, this ) );
}

void HistogrammerProcessor::join() {
    if( thread_ptr->joinable() ) thread_ptr->join();
}

void HistogrammerProcessor::process() {
    while( true ) {
        std::unique_lock<std::mutex> lk(mtx);
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
        Fei4Data *data = dynamic_cast<Fei4Data*>(d.get());
        if (data == nullptr)
            continue;
        for (unsigned i=0; i<algorithms.size(); i++) {
            algorithms[i]->create(data->lStat);
            algorithms[i]->processEvent(data);
        }
        this->publish();
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
