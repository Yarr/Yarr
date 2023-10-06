// #################################
// # Author: Timon Heim
// # Email: timon.heim at cern.ch
// # Project: Yarr
// # Description: Analysis Base class
// # Comment: Moved to libYarr by Bruce Gallop
// ################################

#include "AnalysisAlgorithm.h"

#include "logging.h"

#include "StdParameterAction.h"

namespace {
    auto alog = logging::make_log("AnalysisAlgorithm");
}

bool AnalysisAlgorithm::isPOILoop(const LoopActionBaseInfo *l) {
    // Determine if a given loop l is iterating a parameter that is in the m_parametersOfInterest

    if(!l->isParameterLoop()) {
        return false;
    }

    auto spl = dynamic_cast<const StdParameterAction*>(l);
    if(!spl) {
        // Not a StdParameterLoop loop
        // Other parameter loops do not derive from StdParameterLoop.

        // This basically indicates that the parameter of interest search
        // is not needed, so all loops are of interest.
        // For now, just return true and let the decision be made elsewhere
        return true;
    }

    auto loop_name = spl->getParName();

    // m_parametersOfInterest is set in AnalysisAlgorithm::loadConfig
    if ( m_parametersOfInterest.empty() ) {
        // m_parametersOfInterest is not set, treat any parameter loop as POI loop
        return true;
    } else {
        for (const auto& poi : m_parametersOfInterest) {
            if (loop_name == poi)
                return true;
        }

        // The parameter is this loop is not a parameter of interest
        return false;
    }
}

AnalysisProcessor::AnalysisProcessor() = default;

AnalysisProcessor::AnalysisProcessor(Bookkeeper *b, unsigned uid)
  : bookie(b), id(uid)
{
}

AnalysisProcessor::~AnalysisProcessor() = default;

void AnalysisProcessor::init() {
    for (unsigned i=0; i<algorithms.size(); i++) {
        algorithms[i]->connect(output, feedback);
        algorithms[i]->init(scan);
    }
}

void AnalysisProcessor::run() {
    SPDLOG_LOGGER_TRACE(alog, "");
    thread_ptr.reset( new std::thread( &AnalysisProcessor::process, this ) );
}

void AnalysisProcessor::loadConfig(const json &j){
    for (unsigned i=0; i<algorithms.size(); i++) {
        if (j.contains({std::to_string(i),"config"})) {
	    algorithms[i]->loadConfig(j[std::to_string(i)]["config"]);
	}
    }
}

void AnalysisProcessor::join() {
    if( thread_ptr->joinable() ) thread_ptr->join();
}

void AnalysisProcessor::process() {
    while( true ) {

        input->waitNotEmptyOrDone();

        std::this_thread::sleep_for(std::chrono::milliseconds(200));
        process_core();

        if( input->isDone() ) {
            std::this_thread::sleep_for(std::chrono::milliseconds(200));
            process_core();  // this line is needed if the data comes in before scanDone is changed.
            alog->info("Analysis done!");
            break;
        }
    }

    process_core();

    end();

}

void AnalysisProcessor::process_core() {
    while(!input->empty()) {
        auto h = input->popData();
        if (h != nullptr) {
            for (unsigned i=0; i<algorithms.size(); i++) {
                algorithms[i]->processHistogram(&*h);
            }
        }
        // Pass input histogram to output if needed
        if (storeInputHisto) output->pushData(std::move(h));
    }
}

void AnalysisProcessor::end() {
    SPDLOG_LOGGER_TRACE(alog, "");
    for (unsigned i=0; i<algorithms.size(); i++) {
        algorithms[i]->end();
    }
}

void AnalysisProcessor::addAlgorithm(std::unique_ptr<AnalysisAlgorithm> a) {
    a->setBookkeeper(bookie);
    a->setId(id);
    algorithms.push_back(std::move(a));
}
