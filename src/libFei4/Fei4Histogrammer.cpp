// #################################
// # Author: Timon Heim
// # Email: timon.heim at cern.ch
// # Project: Yarr
// # Description: Histograms Fei4 data
// # Comment: 
// ################################

#include "Fei4Histogrammer.h"

bool Fei4Histogrammer::processorDone = false;

Fei4Histogrammer::Fei4Histogrammer() {
}

Fei4Histogrammer::~Fei4Histogrammer() {
    for (unsigned i=0; i<algorithms.size(); i++)
        delete algorithms[i];
}

void Fei4Histogrammer::init() {
    processorDone = false;
}

void Fei4Histogrammer::clearHistogrammers() {
    for(unsigned int i = 0; i < algorithms.size(); i++) {
        delete (algorithms.at(i));
    }
    algorithms.clear();
}


void Fei4Histogrammer::run() {
    thread_ptr.reset( new std::thread( &Fei4Histogrammer::process, this ) );
}

void Fei4Histogrammer::join() {
    if( thread_ptr->joinable() ) thread_ptr->join();
}

void Fei4Histogrammer::process() {
    while( true ) {

        //std::cout << __PRETTY_FUNCTION__ << std::endl;

        std::unique_lock<std::mutex> lk(mtx);
        input->cv.wait( lk, [&] { return processorDone || !input->empty(); } );

        std::this_thread::sleep_for(std::chrono::milliseconds(200));
        process_core();
        output->cv.notify_all();  // notification to the downstream

        if( processorDone ) {
            std::this_thread::sleep_for(std::chrono::milliseconds(200));
            process_core();  // this line is needed if the data comes in before scanDone is changed.
            std::cout << __PRETTY_FUNCTION__ << ": processorDone!" << std::endl;
            output->cv.notify_all();  // notification to the downstream
            break;
        }
    }

    process_core();
    output->cv.notify_all();  // notification to the downstream

}

void Fei4Histogrammer::process_core() {
    while (!input->empty()) {
        Fei4Data *data = dynamic_cast<Fei4Data*>(input->popData());
        if (data == NULL)
            continue;
        for (unsigned i=0; i<algorithms.size(); i++) {
            algorithms[i]->create(data->lStat);
            algorithms[i]->processEvent(data);
        }
        delete data;
        this->publish();
    }
}

void Fei4Histogrammer::publish() {
    for (unsigned i=0; i<algorithms.size(); i++) {
        if (algorithms[i]->getHisto() != NULL) {
            output->pushData(algorithms[i]->getHisto());
        }
    }
}

void Fei4Histogrammer::toFile(std::string basename) {
    for (std::deque<HistogramBase*>::iterator it = output->begin(); it != output->end(); ++it) {
        std::cout << "Saving : " << (*it)->getName() << std::endl;
        (*it)->toFile(basename);
    }
}

void Fei4Histogrammer::plot(std::string basename) {
    for (std::deque<HistogramBase*>::iterator it = output->begin(); it != output->end(); ++it) {
        std::cout << "Plotting : " << (*it)->getName() << std::endl;
        (*it)->plot(basename);
    }    
}

void DataArchiver::processEvent(Fei4Data *data) {
    for (std::list<Fei4Event>::iterator eventIt = (data->events).begin(); eventIt!=data->events.end(); ++eventIt) {   
        Fei4Event curEvent = *eventIt;
        // Save Event to File
        curEvent.toFileBinary(fileHandle);
    }
}


void OccupancyMap::processEvent(Fei4Data *data) {
    for (std::list<Fei4Event>::iterator eventIt = (data->events).begin(); eventIt!=data->events.end(); ++eventIt) {   
        Fei4Event curEvent = *eventIt;
        for (std::list<Fei4Hit>::iterator hitIt = curEvent.hits.begin(); hitIt!=curEvent.hits.end(); ++hitIt) {   
            Fei4Hit curHit = *hitIt;
            if(curHit.tot > 0)
                h->fill(curHit.col, curHit.row);
        }
    }
}

void TotMap::processEvent(Fei4Data *data) {
    for (std::list<Fei4Event>::iterator eventIt = (data->events).begin(); eventIt!=data->events.end(); ++eventIt) {   
        Fei4Event curEvent = *eventIt;
        for (std::list<Fei4Hit>::iterator hitIt = curEvent.hits.begin(); hitIt!=curEvent.hits.end(); ++hitIt) {   
            Fei4Hit curHit = *hitIt;
            if(curHit.tot > 0)
                h->fill(curHit.col, curHit.row, curHit.tot);
        }
    }
}

void Tot2Map::processEvent(Fei4Data *data) {
    for (std::list<Fei4Event>::iterator eventIt = (data->events).begin(); eventIt!=data->events.end(); ++eventIt) {   
        Fei4Event curEvent = *eventIt;
        for (std::list<Fei4Hit>::iterator hitIt = curEvent.hits.begin(); hitIt!=curEvent.hits.end(); ++hitIt) {   
            Fei4Hit curHit = *hitIt;
            if(curHit.tot > 0)
                h->fill(curHit.col, curHit.row, curHit.tot*curHit.tot);
        }
    }
}

void TotDist::processEvent(Fei4Data *data) {
    for (std::list<Fei4Event>::iterator eventIt = (data->events).begin(); eventIt!=data->events.end(); ++eventIt) {   
        Fei4Event curEvent = *eventIt;
        for (std::list<Fei4Hit>::iterator hitIt = curEvent.hits.begin(); hitIt!=curEvent.hits.end(); ++hitIt) {   
            Fei4Hit curHit = *hitIt;
            if(curHit.tot > 0)
                h->fill(curHit.tot);
        }
    }
}

void L1Dist::processEvent(Fei4Data *data) {
    // Event Loop
    for (std::list<Fei4Event>::iterator eventIt = (data->events).begin(); eventIt!=data->events.end(); ++eventIt) {   
        Fei4Event curEvent = *eventIt;
        if(curEvent.l1id != l1id) {
            l1id = curEvent.l1id;
            bcid_offset = curEvent.bcid;
        }
        int delta_bcid = curEvent.bcid - bcid_offset;
        if (delta_bcid < 0)
            delta_bcid += 0x400;
        h->fill(delta_bcid, curEvent.nHits);
    }
}

void HitsPerEvent::processEvent(Fei4Data *data) {
    // Event Loop
    for (std::list<Fei4Event>::iterator eventIt = (data->events).begin(); eventIt!=data->events.end(); ++eventIt) {   
        Fei4Event curEvent = *eventIt;
        h->fill(curEvent.nHits);
    }
}
