// #################################
// # Author: Timon Heim
// # Email: timon.heim at cern.ch
// # Project: Yarr
// # Description: RD53A Data Processor
// # Date: Apr 2018
// ################################

#include "Rd53aDataProcessor.h"
#include "AllProcessors.h"
#include "EventData.h"

#include "logging.h"

namespace {
  auto logger = logging::make_log("Rd53aDataProcessor");
}

bool rd53a_proc_registered =
    StdDict::registerDataProcessor("RD53A", []() { return std::unique_ptr<FeDataProcessor>(new Rd53aDataProcessor());});

Rd53aDataProcessor::Rd53aDataProcessor()  {
    m_input = NULL;
}

Rd53aDataProcessor::~Rd53aDataProcessor() = default;

void Rd53aDataProcessor::init() {
    SPDLOG_LOGGER_TRACE(logger, "");
}

void Rd53aDataProcessor::run() {
    SPDLOG_LOGGER_TRACE(logger, "");

    thread_ptr.reset(new std::thread(&Rd53aDataProcessor::process, this));
}

void Rd53aDataProcessor::join() {
    thread_ptr->join();
}

void Rd53aDataProcessor::process() {
    while(true) {
        m_input->waitNotEmptyOrDone();

        process_core();
        // TODO the timing on these seems sensitive
        std::this_thread::sleep_for(std::chrono::microseconds(200));
        if( m_input->isDone() ) {
            std::this_thread::sleep_for(std::chrono::microseconds(200));
            process_core(); // this line is needed if the data comes in before done_flag is changed.
            break;
        }
    }

    process_core();
}

void Rd53aDataProcessor::process_core() {
    // TODO put data from channels back into input, so other processors can use it
    tag = 666;
    l1id = 666;
    bcid = 666;
    wordCount = 0;
    hits = 0;

    unsigned dataCnt = 0;
    while(!m_input->empty()) {
        // Get data containers
        auto curInV = m_input->popData();
        if (curInV == nullptr)
            continue;

        // Create Output Container
        std::unique_ptr<FrontEndData> curOut(new FrontEndData(curInV->stat));
        int events = 0;

        unsigned size = curInV->size();
        
        // Forward empty end of loop event
        if (size == 0 && curInV->stat.is_end_of_iteration) {
                curOut = std::make_unique<FrontEndData>(curInV->stat);
                m_output->pushData(std::move(curOut));
                continue;
        }
        
        for(unsigned c=0; c<size; c++) {
            RawDataPtr curIn = curInV->data[c];
            // Process
            unsigned words = curIn->getSize();
            dataCnt += words;
            for (unsigned i=0; i<words; i++) {
                // Decode content
                // TODO this needs review, can't deal with user-k data
                uint32_t data = curIn->get(i);

                // TODO get channel from parent
                unsigned channel = 0;
                //unsigned channel = activeChannels[(i/2)%activeChannels.size()];
                logger->debug("[{}]\t\t[{}] = 0x{:x}", i, channel, data);
                if (__builtin_expect(((data & 0xFFFF0000) != 0xFFFF0000 ), 1)) {
                    if ((data >> 25) & 0x1) { // is header
                        l1id = 0x1F & (data >> 20);
                        tag = 0x1F & (data >> 15);
                        bcid = 0x7FFF & data;
                        // Create new event
                        curOut->newEvent(tag, l1id, bcid);
                        events++;
                        sendFeedback(tag, bcid);
                        //logger->debug("[Header] : L1ID({}) TAG({}) BCID({})", l1id[channel], tag[channel], bcid[channel]);
                    } else { // is hit data
                        unsigned core_col = 0x3F & (data >> 26);
                        unsigned core_row = 0x3F & (data >> 20);
                        unsigned region = 0xF & (data >> 16);
                        unsigned tot0 = 0xF & (data >> 0); //left most
                        unsigned tot1 = 0xF & (data >> 4);
                        unsigned tot2 = 0xF & (data >> 8);
                        unsigned tot3 = 0xF & (data >> 12);

                        unsigned pix_col = core_col*8+((region&0x1)*4);
                        unsigned pix_row = core_row*8+(0x7&(region>>1));
                        //logger->debug("[Data] : COL({}) ROW({}) Region({}) TOT({},{},{},{}) RAW(0x{:x})", core_col, core_row, region, tot3, tot2, tot1, tot0, data);

                        if (__builtin_expect((pix_col < Rd53a::n_Col && pix_row < Rd53a::n_Row), 1)) {
                            // Check if there is already an event
                            if (events == 0) {
                                logger->debug("[{}] No header in data fragment!", channel);
                                curOut->newEvent(666, l1id, bcid);
                                events++;
                            }
                            // TODO Make decision on pixel address start 0,0 or 1,1
                            pix_row++;
                            pix_col++;
                            if (tot0 != 0xF) {
                                curOut->curEvent->addHit(pix_row, pix_col, tot0+1);
                                hits++;
                            }
                            if (tot1 != 0xF) {
                                curOut->curEvent->addHit(pix_row, pix_col+1, tot1+1);
                                hits++;
                            }
                            if (tot2 != 0xF) {
                                curOut->curEvent->addHit(pix_row, pix_col+2, tot2+1);
                                hits++;
                            }
                            if (tot3 != 0xF) {
                                curOut->curEvent->addHit(pix_row, pix_col+3, tot3+1);
                                hits++;
                            }
                        } else {
                            logger->error("[{}] Received data not valid: 0x{:x}", channel, curIn->getSize());
                        }

                    }
                }
            }            
        }

        // Push data out
        if (events > 0) {
            m_output->pushData(std::move(curOut));
        }
    }

}

void Rd53aDataProcessor::sendFeedback(unsigned tag, unsigned bcid)
{
    std::unique_ptr<FeedbackProcessingInfo> stat(new FeedbackProcessingInfo{.trigger_tag = PROCESSING_FEEDBACK_TRIGGER_TAG_ERROR});
    FeedbackProcessingInfo &curStatus = *stat;
    curStatus.trigger_tag = tag;
    curStatus.bcid = bcid;
    if (statusFb != nullptr) statusFb->pushData(std::move(stat));

    return;
}
