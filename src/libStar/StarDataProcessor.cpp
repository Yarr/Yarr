#include "StarDataProcessor.h"

#include <bitset>
#include <iostream>

#include "AllProcessors.h"
#include "LoopStatus.h"

#include "StarChipPacket.h"
#include "StarCfg.h"

#include "EventData.h"

#include "logging.h"

namespace {
  auto logger = logging::make_log("StarDataProcessor");
}

void process_data(RawData &curIn,
                  FrontEndData &curOut,
                  FeedbackProcessingInfo &curStatus,
                  const std::array<uint8_t, 11> &chip_map);

bool star_proc_registered =
  StdDict::registerDataProcessor("Star", []() { return std::unique_ptr<DataProcessor>(new StarDataProcessor());});
bool star_proc_registered_0 =
  StdDict::registerDataProcessor("Star_vH0A0", []() { return std::unique_ptr<DataProcessor>(new StarDataProcessor());});
bool star_proc_registered_ppa =
  StdDict::registerDataProcessor("Star_vH0A1", []() { return std::unique_ptr<DataProcessor>(new StarDataProcessor());});
bool star_proc_registered_ppb =
  StdDict::registerDataProcessor("Star_vH1A1", []() { return std::unique_ptr<DataProcessor>(new StarDataProcessor());});

StarDataProcessor::StarDataProcessor()
  : DataProcessor()
{}

StarDataProcessor::~StarDataProcessor() = default;

void StarDataProcessor::init() {

}

void StarDataProcessor::connect(FrontEndCfg *feCfg, ClipBoard<RawDataContainer> *arg_input, ClipBoard<EventDataBase> *arg_output) {
  if(feCfg == nullptr) {
    throw std::runtime_error("StarDataProcessor::connect given null config");
  }
  StarCfg *cfg = dynamic_cast<StarCfg*>(feCfg);
  if(cfg == nullptr) {
    throw std::runtime_error("StarDataProcessor::connect given bad config (not StarCfg");
  }

  chip_map = cfg->hcc().histoChipMap();

  logger->debug("Map loaded from config: {} {} {} {} {} {} {} {} {} {} {}",
                chip_map[0], chip_map[1], chip_map[2],
                chip_map[3], chip_map[4], chip_map[5],
                chip_map[6], chip_map[7], chip_map[8],
                chip_map[9], chip_map[10]);

  input = arg_input;
  output = arg_output;
}

void StarDataProcessor::run() {
    thread_ptr.reset( new std::thread(&StarDataProcessor::process, this));
}

void StarDataProcessor::join() {
    thread_ptr->join();
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
        if (curInV == nullptr)
            continue;

        // Create Output Container
        std::unique_ptr<FrontEndData> curOut(new FrontEndData(curInV->stat));

        unsigned size = curInV->size();

        for(unsigned c=0; c<size; c++) {
            RawDataPtr r = curInV->data[c];
            unsigned channel = r->getAdr(); //elink number
            std::unique_ptr<FeedbackProcessingInfo> stat(new FeedbackProcessingInfo{.trigger_tag = PROCESSING_FEEDBACK_TRIGGER_TAG_ERROR});
            process_data(*r, *curOut, *stat, chip_map);
            if (statusFb != nullptr) statusFb->pushData(std::move(stat));
        }

        output->pushData(std::move(curOut));
        // dataCnt++;
    }
}

void process_data(RawData &curIn,
                  FrontEndData &curOut,
                  FeedbackProcessingInfo &curStatus,
                  const std::array<uint8_t, 11> &chip_map) {
    StarChipPacket packet;
    curStatus.packet_size = curIn.getSize();

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
        auto l0id = packet.l0id;
        auto l1id = packet.l0id;
        auto bcid = packet.bcid;

        curStatus.trigger_tag = l0id;
        curStatus.n_clusters  = packet.n_clusters();
        curStatus.bcid        = bcid;
        if (packet.n_clusters()==0) return; //empty packet

        curOut.newEvent(l0id, l1id, bcid);

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
        curStatus.trigger_tag = PROCESSING_FEEDBACK_TRIGGER_TAG_RR;

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
        curStatus.trigger_tag = PROCESSING_FEEDBACK_TRIGGER_TAG_Control;
        if(logger->should_log(spdlog::level::trace)) {
            std::stringstream os;
            packet.print_clusters(os);
            logger->trace("{}", os.str());
        }
    }
}

// Need to instantiate something to register the logger
bool parser_logger_registered = [](){ StarChipPacket::make_logger(); return true; }();
