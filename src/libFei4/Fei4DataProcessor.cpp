#include "AllProcessors.h"
#include "Fei4DataProcessor.h"
#include "LoopStatus.h"

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

Fei4DataProcessor::~Fei4DataProcessor() {

}

void Fei4DataProcessor::init() {
    SPDLOG_LOGGER_TRACE(flog, "");
    for(std::map<unsigned, ClipBoard<EventDataBase> >::iterator it = outMap->begin(); it != outMap->end(); ++it) {
        activeChannels.push_back(it->first);
    }
}

void Fei4DataProcessor::run() {
    SPDLOG_LOGGER_TRACE(flog, "");
    const unsigned int numThreads = std::thread::hardware_concurrency();
    for (unsigned i=0; i<numThreads; i++) {
        thread_ptrs.emplace_back( new std::thread(&Fei4DataProcessor::process, this) );
        flog->info("  -> Processor thread #{} started!", i);
    }
}

void Fei4DataProcessor::join() {
    for( auto& thread : thread_ptrs ) {
        if( thread->joinable() ) thread->join();
    }
}


void Fei4DataProcessor::process() {
    while(true) {
        std::unique_lock<std::mutex> lk(mtx);
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
    for (unsigned i=0; i<activeChannels.size(); i++) {
        tag[activeChannels[i]] = 0;
        l1id[activeChannels[i]] = 0;
        bcid[activeChannels[i]] = 0;
        wordCount[activeChannels[i]] = 0;
        hits[activeChannels[i]] = 0;
    }

    unsigned dataCnt = 0;
    while(!input->empty()) {
        // Get data containers
        std::unique_ptr<RawDataContainer> curInV(input->popData());
        if (curInV == nullptr)
            continue;

        // Create Output Container
        std::map<unsigned, std::unique_ptr<Fei4Data>> curOut;
        std::map<unsigned, int> events;
        for (unsigned i=0; i<activeChannels.size(); i++) {
            curOut[activeChannels[i]].reset(new Fei4Data(curInV->stat));
            events[activeChannels[i]] = 0;
        }

        unsigned size = curInV->size();
        //if (size == 0)
        //std::cout << "Empty!" << std::endl;
        for(unsigned c=0; c<size; c++) {
            RawData *curIn = new RawData(curInV->adr[c], curInV->buf[c], curInV->words[c]);
            // Process
            unsigned words = curIn->words;
            for (unsigned i=0; i<words; i++) {
                uint32_t value = curIn->buf[i];
                uint32_t header = ((value & 0x00FF0000) >> 16);
                unsigned channel = ((value & 0xFC000000) >> 26);
                unsigned type = ((value &0x03000000) >> 24);
                if (type == 0x1) {
                    tag[channel] = unsigned(value & 0x00FFFFFF);
                } else if (type == 0x3) {
                    // skip
                } else if (type == 0x0) {
                    wordCount[channel]++;
                    if (__builtin_expect((value == 0xDEADBEEF), 0)) {
                        flog->error("[{}] Noticed readout error: 0x{:x}", channel, value);
                    } else if (__builtin_expect((curOut[channel] == NULL), 0)) {
                        flog->error("Received data for channel {} but storage not initiliazed!", channel);
                    } else if (header == 0xe9) {
                        // Pixel Header
                        l1id[channel] = (value & 0x7c00) >> 10;
                        bcid[channel] = (value & 0x03FF);
                        curOut[channel]->newEvent(tag[channel], l1id[channel], bcid[channel]);

                        events[channel]++;
                    } else if (header == 0xef) {
                        // Service Record
                        unsigned code = (value & 0xFC00) >> 10;
                        unsigned number = value & 0x03FF;
                        curOut[channel]->serviceRecords[code]+=number;
                        //} else if (header == 0xea) {
                        // Address Record
                        //} else if (header == 0xec) {
                        // Value Record
                } else {
                    uint16_t col = (value & 0xFE0000) >> 17;
                    uint16_t row = (value & 0x01FF00) >> 8;
                    uint8_t tot1 = (value & 0xF0) >> 4;
                    uint8_t tot2 = (value & 0xF);
                    if (events[channel] == 0 ) {
                        flog->warn("[{}] No header in data fragment!", channel);
                        curOut[channel]->newEvent(0xDEADBEEF, l1id[channel], bcid[channel]);
                        events[channel]++;
                        //hits[channel] = 0;
                    }
                    if (__builtin_expect((col == 0 || row == 0 || col > 80 || row > 336), 0)) {
                        badCnt++;
                        flog->error("[{}] Received data (0x{:x})out of bounds and not valid, probably due to readout errors!", channel, value);
                    } else {
                        unsigned dec_tot1 = totCode[hitDiscCfg][tot1];
                        unsigned dec_tot2 = totCode[hitDiscCfg][tot2];
                        if (dec_tot1 > 0) {
                            curOut[channel]->curEvent->addHit(row, col, dec_tot1);
                            hits[channel]++;
                        }
                        if (dec_tot2 > 0) {
                            curOut[channel]->curEvent->addHit(row+1, col, dec_tot2);
                            hits[channel]++;
                        }
                    }
                }
                }
                if (badCnt > 10)
                    break;
            }
            delete curIn;
        }
        for (unsigned i=0; i<activeChannels.size(); i++) {
            outMap->at(activeChannels[i]).pushData(std::move(curOut[activeChannels[i]]));
        }
        //Cleanup
        dataCnt++;
    }
}
