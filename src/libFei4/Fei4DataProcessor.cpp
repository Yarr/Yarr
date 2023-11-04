#include "AllProcessors.h"
#include "Fei4DataProcessor.h"
#include "LoopStatus.h"
#include "EventData.h"

#include <iostream>

#include "logging.h"

namespace {
    auto flog = logging::make_log("Fei4DataProc");
}

bool fei4_proc_registered =
    StdDict::registerDataProcessor("FEI4B", []() { return std::unique_ptr<DataProcessor>(new Fei4DataProcessor());});

Fei4DataProcessor::Fei4DataProcessor(unsigned arg_hitDiscCfg) : DataProcessor(){
    SPDLOG_LOGGER_TRACE(flog, "");
    input = NULL;
    hitDiscCfg = arg_hitDiscCfg;
    totCode = {{{{1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 14, 0}},
        {{2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 1, 0}},
        {{3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 1, 0}}}};


}

Fei4DataProcessor::~Fei4DataProcessor() = default;

void Fei4DataProcessor::init() {
    SPDLOG_LOGGER_TRACE(flog, "");
}

void Fei4DataProcessor::run() {
    thread_ptr.reset( new std::thread(&Fei4DataProcessor::process, this) );
}

void Fei4DataProcessor::join() {
    thread_ptr->join();
}


void Fei4DataProcessor::process() {
    while(true) {
        input->waitNotEmptyOrDone();

        process_core();

        if( input->isDone() ) {
            process_core(); // this line is needed if the data comes in before done flag is changed.
            break;
        }
    }

    process_core();
}

void Fei4DataProcessor::process_core() {
    // TODO put data from channels back into input, so other processors can use it
    unsigned badCnt = 0;
    tag = 0;
    l1id = 0;
    bcid = 0;
    wordCount = 0;
    hits = 0;

    unsigned dataCnt = 0;
    while(!input->empty()) {
        // Get data containers
        std::unique_ptr<RawDataContainer> curInV(input->popData());
        if (curInV == nullptr)
            continue;

        // Create Output Container
        std::unique_ptr<FrontEndData> curOut(new FrontEndData(curInV->stat));;
        int events = 0;

        unsigned size = curInV->size();
        // Special case for empty events, could be end of loop
        if (size == 0 && curInV->stat.is_end_of_iteration) {
                curOut = std::make_unique<FrontEndData>(curInV->stat);
                output->pushData(std::move(curOut));
                continue;
        }

        //if (size == 0)
        //std::cout << "Empty!" << std::endl;
        for(unsigned c=0; c<size; c++) {
            RawDataPtr curIn = curInV->data[c];
            // Process
            unsigned words = curIn->getSize();
            for (unsigned i=0; i<words; i++) {
                uint32_t value = curIn->get(i);
                uint32_t header = ((value & 0x00FF0000) >> 16);
                // TODO channel should come from inheritance not data
                unsigned channel = ((value & 0xFC000000) >> 26);
                unsigned type = ((value &0x03000000) >> 24);
                if (type == 0x1) {
                    tag = unsigned(value & 0x00FFFFFF);
                } else if (type == 0x3) {
                    // skip
                } else if (type == 0x0) {
                    wordCount++;
                    if (__builtin_expect((value == 0xDEADBEEF), 0)) {
                        flog->error("[{}] Noticed readout error: 0x{:x}", channel, value);
                    } else if (__builtin_expect((curOut == NULL), 0)) {
                        flog->error("Received data for channel {} but storage not initiliazed!", channel);
                    } else if (header == 0xe9) {
                        // Pixel Header
                        l1id = (value & 0x7c00) >> 10;
                        bcid = (value & 0x03FF);
                        curOut->newEvent(tag, l1id, bcid);
                        sendFeedback(l1id, bcid);

                        events++;
                    } else if (header == 0xef) {
                        // Service Record
                        unsigned code = (value & 0xFC00) >> 10;
                        unsigned number = value & 0x03FF;
                        curOut->serviceRecords[code]+=number;
                        //} else if (header == 0xea) {
                        // Address Record
                        //} else if (header == 0xec) {
                        // Value Record
                } else {
                    uint16_t col = (value & 0xFE0000) >> 17;
                    uint16_t row = (value & 0x01FF00) >> 8;
                    uint8_t tot1 = (value & 0xF0) >> 4;
                    uint8_t tot2 = (value & 0xF);
                    if (events == 0 ) {
                        flog->warn("[{}] No header in data fragment!", channel);
                        curOut->newEvent(0xDEADBEEF, l1id, bcid);
                        events++;
                        //hits[channel] = 0;
                    }
                    if (__builtin_expect((col == 0 || row == 0 || col > 80 || row > 336), 0)) {
                        badCnt++;
                        flog->error("[{}] Received data (0x{:x})out of bounds and not valid, probably due to readout errors!", channel, value);
                    } else {
                        unsigned dec_tot1 = totCode[hitDiscCfg][tot1];
                        unsigned dec_tot2 = totCode[hitDiscCfg][tot2];
                        if (dec_tot1 > 0) {
                            curOut->curEvent->addHit(row, col, dec_tot1);
                            hits++;
                        }
                        if (dec_tot2 > 0) {
                            curOut->curEvent->addHit(row+1, col, dec_tot2);
                            hits++;
                        }
                    }
                }
                }
                if (badCnt > 10)
                    break;
            }
        }
        output->pushData(std::move(curOut));
        //Cleanup
        dataCnt++;
    }
}

void Fei4DataProcessor::sendFeedback(unsigned tag, unsigned bcid)
{
    std::unique_ptr<FeedbackProcessingInfo> stat(new FeedbackProcessingInfo{.trigger_tag = PROCESSING_FEEDBACK_TRIGGER_TAG_ERROR});
    FeedbackProcessingInfo &curStatus = *stat;
    curStatus.trigger_tag = tag;
    curStatus.bcid = bcid;
    if (statusFb != nullptr) statusFb->pushData(std::move(stat));

    return;
}
