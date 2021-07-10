#include "StarDataProcessor.h"

#include <bitset>
#include <iostream>

#include "AllProcessors.h"
#include "LoopStatus.h"

#include "StarChipPacket.h"

#include "EventData.h"

#include "logging.h"

namespace {
  auto logger = logging::make_log("StarDataProcessor");
}

void process_data(RawData &curIn,
                  FrontEndData &curOut);

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
        logger->info("  -> Processor thread #{} started!", i);
    }
}

void StarDataProcessor::join() {
    for( auto& thread : thread_ptrs ) {
        if( thread->joinable() ) thread->join();
    }
}

void StarDataProcessor::process() {
    while(true) {
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
        std::map<unsigned, std::unique_ptr<FrontEndData>> curOut;
        for (unsigned i=0; i<activeChannels.size(); i++) {
            curOut[activeChannels[i]].reset(new FrontEndData(curInV->stat));
        }

        unsigned size = curInV->size();

        for(unsigned c=0; c<size; c++) {
            RawData r(curInV->adr[c], curInV->buf[c], curInV->words[c]);
            unsigned channel = curInV->adr[c]; //elink number
            if(!curOut[channel]) {
              logger->warn("Channel {} not found", channel);
              for (unsigned i=0; i<activeChannels.size(); i++) {
                logger->warn(" Active channel {} is {}", i, activeChannels[i]);
              }
              continue;
            }
            process_data(r, *curOut[channel]);
        }

        for (unsigned i=0; i<activeChannels.size(); i++) {
            outMap->at(activeChannels[i]).pushData(std::move(curOut[activeChannels[i]]));
        }
        // dataCnt++;
    }
}

void process_data(RawData &curIn,
                  FrontEndData &curOut) {
    StarChipPacket packet;

    packet.add_word(0x13C); //add SOP, only to make decoder happy
    for(unsigned iw=0; iw<curIn.words; iw++) {
        for(int i=0; i<4;i++){
            packet.add_word((curIn.buf[iw]>>i*8)&0xFF);
        }
    }
    packet.add_word(0x1DC); //add EOP, only to make decoder happy

    if(packet.parse()) {
      logger->error("Star packet parsing failed, continuing with the extracted data\n");
    }
    
    logger->debug("Process data");
    if(logger->should_log(spdlog::level::trace)) {
      std::stringstream os;
      packet.print_clusters(os);
      logger->trace("{}", os.str());
    }

    PacketType packetType = packet.getType();
    if(packetType == TYP_LP || packetType == TYP_PR){
        if (packet.n_clusters()==0) return; //empty packet

        int tag = packet.l0id;
        auto l1id = packet.l0id;
        auto bcid = packet.bcid;

        curOut.newEvent(tag, l1id, bcid);

        for(unsigned  ithCluster=0; ithCluster < packet.clusters.size(); ++ithCluster){
            Cluster cluster = packet.clusters.at(ithCluster);

            int row = ((cluster.address>>7)&1)+1;

            // Split hits into two rows of strips
            curOut.curEvent->addHit( row,
                                     cluster.input_channel*128+((cluster.address&0x7f)+1), 1);
            //NOTE::tot(1) is just dummy value, because of """if(curHit.tot > 0)""" in Fei4Histogrammer::XXX::processEvent(Fei4Data *data)
            //row and col both + 1 because pixel row & col numbering start from 1, see Fei4Histogrammer & Fei4Analysis

            std::bitset<3> nextPattern (cluster.next);
            for(unsigned i=0; i<3; i++){
                if(!nextPattern.test(i)) continue;
                auto nextAddress = cluster.address+(3-i);
                curOut.curEvent->addHit( row,
                                         cluster.input_channel*128+((nextAddress&0x7f)+1),1);

                // It's an error for cluster to escape either "side"
                if((cluster.address & (~0x7f)) != (nextAddress & (~0x7f))) {
                    logger->warn(" strip address > 128");
                }
            }
        }
    } else if(packetType == TYP_ABC_RR || packetType == TYP_HCC_RR) {
        //Assume we don't want to see hit counter reads but want to see other RR's
        if(logger->should_log(spdlog::level::debug) || packet.address < 0x80 || packet.address > 0xbf) {
            packet.print_more(std::cout);
        }
        if(packet.address >= 0x80 && packet.address <= 0xbf) {
            //Hit Counter Register Read
            if (packet.value == 0) return; //No Hits
            logger->trace("Adding hits from HitCounter",packet.address);
              
            curOut.newEvent(0,0,0); //No l0id or bcid
            int start_channel = (packet.address - 0x80)*4;
            for (int i=0; i < 4; i++) {
                int channel = start_channel+i;
                int row = (channel&1)+1;
                int hits = (packet.value>>(8*i)) & 0xff;
                for(int j=0; j<hits; j++)
                    curOut.curEvent->addHit( row,
                                             packet.channel_abc*128+( ((channel>>1)&0x7f)+1), 1);
            }
        }
    } else if (packetType == TYP_ABC_HPR || packetType == TYP_HCC_HPR) {
        if(logger->should_log(spdlog::level::trace)) {
            std::stringstream os;
            packet.print_clusters(os);
            logger->trace("{}", os.str());
        }
    }
}

// Need to instantiate something to register the logger
bool parser_logger_registered = [](){ StarChipPacket::make_logger(); return true; }();
