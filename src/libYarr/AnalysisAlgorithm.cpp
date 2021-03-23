// #################################
// # Author: Timon Heim
// # Email: timon.heim at cern.ch
// # Project: Yarr
// # Description: Analysis Base class
// # Comment: Moved to libYarr by Bruce Gallop
// ################################

#include "AnalysisAlgorithm.h"
#include "AllStdActions.h"

#include "logging.h"

namespace {
    auto alog = logging::make_log("AnalysisAlgorithm");
}

void AnalysisAlgorithm::loadConfig(json &j) {
    if (!j["parametersOfInterest"].empty()) {
        for (unsigned i=0; i<j["parametersOfInterest"].size(); i++) {
            m_parametersOfInterest.push_back(j["parametersOfInterest"][i]);
        }
    }
}

bool AnalysisAlgorithm::isPOILoop(LoopActionBase *l) {
    // Determine if a given loop l is iterating a parameter that is in the m_parametersOfInterest
    // m_parametersOfInterest is set in AnalysisAlgorithm::loadConfig

    // If m_parametersOfInterest is not set, treat all parameter loops as POI loops
    if ( m_parametersOfInterest.empty() and l->isParameterLoop() )
        return true;

    for (const auto& poi : m_parametersOfInterest) {
        if (l->getLabel() == poi)
            return true;
    }

    return false;
}

AnalysisProcessor::AnalysisProcessor() {
}

AnalysisProcessor::AnalysisProcessor(Bookkeeper *b, unsigned ch)
  : bookie(b), channel(ch)
{
}

AnalysisProcessor::~AnalysisProcessor() {
}

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

void AnalysisProcessor::loadConfig(json &j){
    for (unsigned i=0; i<algorithms.size(); i++) {
        if (!j[std::to_string(i)]["config"].empty()){
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
    a->setChannel(channel);
    algorithms.push_back(std::move(a));
}
