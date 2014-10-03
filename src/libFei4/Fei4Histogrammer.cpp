// #################################
// # Author: Timon Heim
// # Email: timon.heim at cern.ch
// # Project: Yarr
// # Description: Histograms Fei4 data
// # Comment: 
// ################################

#include "Fei4Histogrammer.h"

Fei4Histogrammer::Fei4Histogrammer() {

}

Fei4Histogrammer::~Fei4Histogrammer() {
    for (unsigned i=0; i<algorithms.size(); i++)
        delete algorithms[i];
}

void Fei4Histogrammer::init() {

}

void Fei4Histogrammer::process() {
    while (!input->empty()) {
        Fei4Data *data = input->popData();
        if (data != NULL) {
            for (unsigned i=0; i<algorithms.size(); i++) {
                algorithms[i]->processEvent(data);
            }
            delete data;
        }
    }
}

void Fei4Histogrammer::publish() {
    for (unsigned i=0; i<algorithms.size(); i++) {
        output->pushData(algorithms[i]->getResult());
    }
}


void OccupancyHistogram::processEvent(Fei4Data *data) {
    for (std::deque<Fei4Event*>::iterator eventIt = (data->events).begin(); eventIt!=data->events.end(); ++eventIt) {   
        Fei4Event *curEvent = *eventIt;
        for (std::deque<Fei4Hit*>::iterator hitIt = curEvent->hits.begin(); hitIt!=curEvent->hits.end(); ++hitIt) {   
            Fei4Hit *curHit = *hitIt;
            if(curHit->tot > 0)
                h->fill(curHit->col, curHit->row);
        }
    }
}
