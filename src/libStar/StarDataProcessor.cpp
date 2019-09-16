#include "StarDataProcessor.h"

#include <bitset>
#include <iostream>

#include "AllProcessors.h"
#include "LoopStatus.h"

// Used to transfer data to histogrammers
#include "Fei4EventData.h"

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
	// TODO put data from channels back into input, so other processors can use it
//	unsigned badCnt = 0;
	for (unsigned i=0; i<activeChannels.size(); i++) {
		tag[activeChannels[i]] = 0;
		l1id[activeChannels[i]] = 0;
		bcid[activeChannels[i]] = 0;
		wordCount[activeChannels[i]] = 0;
		hits[activeChannels[i]] = 0;
	}

//	unsigned dataCnt = 0;
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
        	RawData curIn(curInV->adr[c], curInV->buf[c], curInV->words[c]);
        	StarChipPacket packet(true);

        	unsigned channel =  curIn.adr;
        	for(unsigned iw=0; iw<curIn.words; iw++) {
        		packet.add_word(curIn.buf[iw]);
        	}

        	packet.parse();
//        	std::cout << __PRETTY_FUNCTION__ << ": Data for Channel " << channel << "\n";
//        	packet.print();

        	PacketType packetType = packet.getType();
        	if(packetType == TYP_LP || packetType == TYP_PR){

        		std::cout << "new physics event packet !!!!!!!!!! "<< std::endl;
        		wordCount[channel]++;  //what does this do?
//        		tag[channel] = chipID;
        		l1id[channel] = packet.l0id;
        		bcid[channel] = packet.bcid;
        		curOut[channel]->newEvent( tag[channel], l1id[channel], bcid[channel]);
        		events[channel]++;

//        		for(unsigned int iW=0; iW < clusters.size(); ++iW){
//        		      Cluster cluster = clusters.at(iW);
//        		      std::string next_binary = std::bitset<3>(cluster.next).to_string();
//        		      printf("  %i) InputChannel: %i, Address: 0x%02x, Next Strip Pattern: %s.\n", iW, cluster.input_channel, cluster.address, next_binary.c_str() );
//        		    }
//        		curOut[channel]->curEvent->addHit(abc_cluster_addr[i], 0, 0);
//        		hits[channel]++; //do we need this? Since Fei4EventData.h already has a hit counter.

        	}
        	else if(packetType == TYP_ABC_RR || packetType == TYP_HCC_RR || packetType == TYP_ABC_HPR || packetType == TYP_HCC_HPR)
        		packet.print_RR_HPR();





        }

        for (unsigned i=0; i<activeChannels.size(); i++) {
            outMap->at(activeChannels[i]).pushData(std::move(curOut[activeChannels[i]]));
        }
        // dataCnt++;
    }
}


