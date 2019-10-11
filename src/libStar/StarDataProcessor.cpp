#include "StarDataProcessor.h"

#include <bitset>
#include <iostream>

#include "AllProcessors.h"
#include "LoopStatus.h"

#include "StarChipPacket.h"

// Used to transfer data to histogrammers
#include "Fei4EventData.h"

void process_data(RawData &curIn,
                  Fei4Data &curOut);

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
        input->waitNotEmptyOrDone();

        process_core();

        if( input->isDone() ) {
            process_core(); // this line is needed if the data comes in before scanDone is changed.
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
        for (unsigned i=0; i<activeChannels.size(); i++) {
            curOut[activeChannels[i]].reset(new Fei4Data());
            curOut[activeChannels[i]]->lStat = curInV->stat;
        }

        unsigned size = curInV->size();

        for(unsigned c=0; c<size; c++) {
            RawData r(curInV->adr[c], curInV->buf[c], curInV->words[c]);
            unsigned channel = curInV->adr[c];
            process_data(r, *curOut[channel]);
        }

        for (unsigned i=0; i<activeChannels.size(); i++) {
            outMap->at(activeChannels[i]).pushData(std::move(curOut[activeChannels[i]]));
        }
        // dataCnt++;
    }
}

void process_data(RawData &curIn,
                  Fei4Data &curOut) {
    StarChipPacket packet(true);

    for(unsigned iw=0; iw<curIn.words; iw++) {
        packet.add_word(curIn.buf[iw]);
    }

    packet.parse();
    // std::cout << __PRETTY_FUNCTION__ << ": Data for Channel " << channel << "\n";
    // packet.print();

    PacketType packetType = packet.getType();
    if(packetType == TYP_LP || packetType == TYP_PR){
        int tag = packet.l0id;
        auto l1id = packet.l0id;
        auto bcid = packet.bcid;

        std::cout << "new physics event packet !!!!!!!!!! "<< std::endl;
        curOut.newEvent(tag, l1id, bcid);

//      for(unsigned int iW=0; iW < clusters.size(); ++iW){
//             Cluster cluster = clusters.at(iW);
//             std::string next_binary = std::bitset<3>(cluster.next).to_string();
//             printf("  %i) InputChannel: %i, Address: 0x%02x, Next Strip Pattern: %s.\n", iW, cluster.input_channel, cluster.address, next_binary.c_str() );
//           }
//        curOut[channel]->curEvent->addHit(abc_cluster_addr[i], 0, 0);
//        hits[channel]++; //do we need this? Since Fei4EventData.h already has a hit counter.

    } else if(packetType == TYP_ABC_RR || packetType == TYP_HCC_RR || packetType == TYP_ABC_HPR || packetType == TYP_HCC_HPR) {
        packet.print_more();
    }
}
