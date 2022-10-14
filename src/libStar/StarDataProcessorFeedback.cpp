#include "StarDataProcessorFeedback.h"

#include "AllProcessors.h"
#include "logging.h"
#include "StarChipPacket.h"

//#include <bitset>
//#include <iostream>
//
//#include "LoopStatus.h"

namespace {
  auto logger = logging::make_log("StarDataProcessorFeedback");
}


bool starFeedback_proc_registered =
  StdDict::registerDataProcessor("StarFeedback", []() { return std::unique_ptr<DataProcessor>(new StarDataProcessorFeedback());});

// process_core is exactly the same as in StarDataProcessor
// except it calls the StarDataProcessorFeedback::process_data
void StarDataProcessorFeedback::process_core() {
    while(!input->empty()) {
        // Get data containers
        std::unique_ptr<RawDataContainer> curInV = input->popData();
        if (curInV == nullptr)
            continue;

        // Create Output Container
        std::unique_ptr<FrontEndData> curOut(new FrontEndData(curInV->stat));

        unsigned size = curInV->size();

        for(unsigned c=0; c<size; c++) {
            RawDataPtr r = curInV->data[c];
            unsigned channel = r->getAdr(); //elink number
            process_data(*r, *curOut);
        }

        output->pushData(std::move(curOut));
        // dataCnt++;
    }
}

// StarDataProcessorFeedback::process_data is the same as process_data in StarDataProcessor
// except it pushes trigger tags to the StarDataProcessorFeedback::statusFb
void StarDataProcessorFeedback::process_data(RawData &curIn,
                  FrontEndData &curOut) {
    StarChipPacket packet;
    int trigger_tag = -10;

    packet.add_word(0x13C); //add SOP, only to make decoder happy
    for(unsigned iw=0; iw<curIn.getSize(); iw++) {
        for(int i=0; i<4;i++){
            packet.add_word((curIn[iw]>>i*8)&0xFF);
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
        if (packet.n_clusters()==0) { //empty packet
          FeedbackProcessingInfo stat = {.trigger_tag = -1};
          statusFb->pushData(std::make_unique<FeedbackProcessingInfo>(stat));
          return;
        }

        trigger_tag = packet.l0id;

        auto l1id = packet.l0id;
        auto bcid = packet.bcid;

        curOut.newEvent(trigger_tag, l1id, bcid);

        for(unsigned  ithCluster=0; ithCluster < packet.clusters.size(); ++ithCluster){
            Cluster cluster = packet.clusters.at(ithCluster);

            int row = ((cluster.address>>7)&1)+1;

            if(cluster.input_channel >= HCC_INPUT_CHANNEL_COUNT) {
              logger->warn("Bad input channel {} in cluster",
                           cluster.input_channel);
              continue;
            }
            int histo_chip = chip_map[cluster.input_channel];
            if(histo_chip == HCC_INPUT_CHANNEL_BAD_SLOT) {
              logger->warn("Bad input channel {} missing in config",
                           cluster.input_channel);
              continue;
            }
            logger->trace("Mapped ic {} to histo {}", cluster.input_channel, histo_chip);
            int histo_base = histo_chip * 128;

            // Split hits into two rows of strips
            curOut.curEvent->addHit( row,
                                     histo_base+((cluster.address&0x7f)+1), 1);
            //NOTE::tot(1) is just dummy value, because of """if(curHit.tot > 0)""" in Fei4Histogrammer::XXX::processEvent(Fei4Data *data)
            //row and col both + 1 because pixel row & col numbering start from 1, see Fei4Histogrammer & Fei4Analysis

            std::bitset<3> nextPattern (cluster.next);
            for(unsigned i=0; i<3; i++){
                if(!nextPattern.test(i)) continue;
                auto nextAddress = cluster.address+(3-i);
                curOut.curEvent->addHit( row,
                                         histo_base+((nextAddress&0x7f)+1),1);

                // It's an error for cluster to escape either "side"
                if((cluster.address & (~0x7f)) != (nextAddress & (~0x7f))) {
                    logger->warn(" strip address > 128");
                }
            }
        }
    } else if(packetType == TYP_ABC_RR || packetType == TYP_HCC_RR) {
        //Assume we don't want to see hit counter reads but want to see other RR's
        trigger_tag = -2; // RR tag

        if(logger->should_log(spdlog::level::debug) || packet.address < 0x80 || packet.address > 0xbf) {
            packet.print_more(std::cout);
        }
        if(packet.address >= 0x80 && packet.address <= 0xbf) {
            //Hit Counter Register Read
            if (packet.value == 0) { //return; //No Hits
              FeedbackProcessingInfo stat = {.trigger_tag = -4};
              statusFb->pushData(std::make_unique<FeedbackProcessingInfo>(stat));
              return;
            }
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
        trigger_tag = -3; // HPR tag
        if(logger->should_log(spdlog::level::trace)) {
            std::stringstream os;
            packet.print_clusters(os);
            logger->trace("{}", os.str());
        }
    }

    FeedbackProcessingInfo stat = {.trigger_tag = trigger_tag};
    statusFb->pushData(std::make_unique<FeedbackProcessingInfo>(stat));
}

// Need to instantiate something to register the logger
bool parser_logger_feedback_registered = [](){ StarChipPacket::make_logger(); return true; }();
