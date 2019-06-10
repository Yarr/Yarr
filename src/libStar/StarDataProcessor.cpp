#include "StarDataProcessor.h"

#include <bitset>
#include <iostream>

#include "AllProcessors.h"
#include "LoopStatus.h"

#include "StarChipPacket.h"

// Used to transfer data to histogrammers
#include "Fei4EventData.h"

void process_data(RawData &curIn);

bool star_proc_registered =
  StdDict::registerDataProcessor("Star", []() { return std::unique_ptr<DataProcessor>(new StarDataProcessor());});

StarDataProcessor::StarDataProcessor()
  : DataProcessor()
{}

StarDataProcessor::~StarDataProcessor() {
}

void StarDataProcessor::init() {
    //std::cout << __PRETTY_FUNCTION__ << std::endl;
    for(std::map<unsigned, ClipBoard<EventDataBase> >::iterator it = outMap->begin(); it != outMap->end(); ++it) {
        activeChannels.push_back(it->first);
    }

    scanDone = false;
}

void StarDataProcessor::run() {
    //std::cout << __PRETTY_FUNCTION__ << std::endl;
    const unsigned int numThreads = std::thread::hardware_concurrency();
    for (unsigned i=0; i<numThreads; i++) {
        thread_ptrs.emplace_back( new std::thread(&StarDataProcessor::process, this) );
        std::cout << "  -> Processor thread #" << i << " started!" << std::endl;
    }
}

void StarDataProcessor::join() {
    for( auto& thread : thread_ptrs ) {
        if( thread->joinable() ) thread->join();
    }
}

void StarDataProcessor::process() {
    while(true) {
        std::unique_lock<std::mutex> lk(mtx);
        input->cv.wait( lk, [&] { return scanDone || !input->empty(); } );

        process_core();

        if( scanDone ) {
            process_core(); // this line is needed if the data comes in before scanDone is changed.
            for (unsigned i=0; i<activeChannels.size(); i++) {
                outMap->at(activeChannels[i]).cv.notify_all(); // notification to the downstream
            }
            break;
        }
    }

    process_core();
}

void StarDataProcessor::process_core() {
    while(!input->empty()) {
        // Get data containers
        std::unique_ptr<RawDataContainer> curInV = input->popData();
        if (curInV == NULL)
            continue;

        // Create Output Container
        std::map<unsigned, std::unique_ptr<Fei4Data>> curOut;
        std::map<unsigned, int> events;
        for (unsigned i=0; i<activeChannels.size(); i++) {
            curOut[activeChannels[i]].reset(new Fei4Data());
            curOut[activeChannels[i]]->lStat = curInV->stat;
            events[activeChannels[i]] = 0;
        }

        unsigned size = curInV->size();

        for(unsigned c=0; c<size; c++) {
            RawData r(curInV->adr[c], curInV->buf[c], curInV->words[c]);
            process_data(r);
        }

        for (unsigned i=0; i<activeChannels.size(); i++) {
            outMap->at(activeChannels[i]).pushData(std::move(curOut[activeChannels[i]]));
        }
        //Cleanup
        // dataCnt++;
    }
}

void process_data(RawData &curIn) {
    unsigned channel = curIn.adr;

    StarChipPacket s(true);
    for(unsigned iw=0; iw<curIn.words; iw++) {
        s.add_word(curIn.buf[iw]);
    }

    s.parse();
    std::cout << __PRETTY_FUNCTION__
              << ": Data for Channel " << channel << "\n";
    s.print();
}
