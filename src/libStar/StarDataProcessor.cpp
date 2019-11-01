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
            unsigned channel = curInV->adr[c]; //elink number
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
    StarChipPacket packet;

    packet.add_word(0x13C); //add SOP, only to make decoder happy
    for(unsigned iw=0; iw<curIn.words; iw++) {
        for(int i=0; i<4;i++){
            packet.add_word((curIn.buf[iw]>>i*8)&0xFF);
        }
    }
    packet.add_word(0x1DC); //add EOP, only to make decoder happy

    packet.parse();
    // std::cout << __PRETTY_FUNCTION__ << ": Data for Channel " << channel << "\n";
    // packet.print_more();

    PacketType packetType = packet.getType();
    if(packetType == TYP_LP || packetType == TYP_PR){
        if (packet.n_clusters()==0) return; //empty packet

        int tag = packet.l0id;
        auto l1id = packet.l0id;
        auto bcid = packet.bcid;

        std::cout << "new physics event packet !!!!!!!!!! "<< std::endl;
        curOut.newEvent(tag, l1id, bcid);

        packet.print_clusters(std::cout);
        for(unsigned  ithCluster=0; ithCluster < packet.clusters.size(); ++ithCluster){
            Cluster cluster = packet.clusters.at(ithCluster);

            // Split hits into two rows of strips
            curOut.curEvent->addHit( ((cluster.address>>7)&1)+1,
                                     (cluster.address&0x7f)+1, 1);
            //NOTE::tot(1) is just dummy value, because of """if(curHit.tot > 0)""" in Fei4Histogrammer::XXX::processEvent(Fei4Data *data)
            //row and col both + 1 because pixel row & col numbering start from 1, see Fei4Histogrammer & Fei4Analysis

            std::bitset<3> nextPattern (cluster.next);
            for(unsigned i=0; i<3; i++){
                if(!nextPattern.test(i)) continue;
                auto nextAddress = cluster.address+(3-i);

                curOut.curEvent->addHit( ((nextAddress>>7)&1)+1,
                                         (nextAddress&0x7f)+1,1);

                // It's an error for cluster to escape either "side"
                if(((nextAddress&0x7f)+1) > 128) {
                    std::cout << " address > 128 " << std::endl;
                }
            }
        }
    } else if(packetType == TYP_ABC_RR || packetType == TYP_HCC_RR || packetType == TYP_ABC_HPR || packetType == TYP_HCC_HPR) {
        packet.print_more(std::cout);
    }
}
