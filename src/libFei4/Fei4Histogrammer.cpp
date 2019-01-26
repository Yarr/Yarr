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

void Fei4Histogrammer::publish() {
    for (unsigned i=0; i<algorithms.size(); i++) {
        auto ptr = algorithms[i]->getHisto();
        if(ptr) {
            output->pushData(std::move(ptr));
        }
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

void Tot3d::processEvent(Fei4Data *data) {
    for (std::list<Fei4Event>::iterator eventIt = (data->events).begin(); eventIt!=data->events.end(); ++eventIt) {   
        Fei4Event curEvent = *eventIt;
        for (std::list<Fei4Hit>::iterator hitIt = curEvent.hits.begin(); hitIt!=curEvent.hits.end(); ++hitIt) {   
            Fei4Hit curHit = *hitIt;
            if(curHit.tot > 0)
                h->fill(curHit.col, curHit.row, curHit.tot);
        }
    }
}

void L1Dist::processEvent(Fei4Data *data) {
    // Event Loop
    for (std::list<Fei4Event>::iterator eventIt = (data->events).begin(); eventIt!=data->events.end(); ++eventIt) {   
        Fei4Event curEvent = *eventIt;
        if(curEvent.l1id != l1id) {
            l1id = curEvent.l1id;
            if (curEvent.bcid - bcid_offset > 16) {
                bcid_offset = curEvent.bcid;
                //current_tag++;
            } else if ((curEvent.bcid+32768) - bcid_offset > 16 &&
                       static_cast<int>(curEvent.bcid) - static_cast<int>(bcid_offset) < 0) {
                bcid_offset = curEvent.bcid;
                //current_tag++;
            }
        }


        int delta_bcid = curEvent.bcid - bcid_offset;
        if (delta_bcid < 0)
            delta_bcid += 32768;
        h->fill(delta_bcid, curEvent.nHits);

        //TODO hack to generate proper tag, should come from FE/FW
        //curEvent.tag = current_tag;

    }
}

void L13d::processEvent(Fei4Data *data) {
    for (std::list<Fei4Event>::iterator eventIt = (data->events).begin(); eventIt!=data->events.end(); ++eventIt) {   
        Fei4Event curEvent = *eventIt;
        
        /*if(curEvent.l1id != l1id) {
            l1id = curEvent.l1id;
            if (curEvent.bcid - bcid_offset > 16) {
                bcid_offset = curEvent.bcid;
            } else if ((curEvent.bcid+32768) - bcid_offset > 16 &&
                       static_cast<int>(curEvent.bcid) - static_cast<int>(bcid_offset) < 0) {
                bcid_offset = curEvent.bcid;
            }
        }

        int delta_bcid = curEvent.bcid - bcid_offset;
        if (delta_bcid < 0)
            delta_bcid += 32768;
        */
        for (std::list<Fei4Hit>::iterator hitIt = curEvent.hits.begin(); hitIt!=curEvent.hits.end(); ++hitIt) {   
            Fei4Hit curHit = *hitIt;
            if(curHit.tot > 0)
                h->fill(curHit.col, curHit.row, curEvent.l1id%16);
        }
    }
}

void HitsPerEvent::processEvent(Fei4Data *data) {
    // Event Loop
    for (std::list<Fei4Event>::iterator eventIt = (data->events).begin(); eventIt!=data->events.end(); ++eventIt) {   
        Fei4Event curEvent = *eventIt;
        h->fill(curEvent.nHits);
    }
}
