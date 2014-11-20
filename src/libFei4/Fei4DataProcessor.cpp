#include "Fei4DataProcessor.h"

#include <iostream>

#include "LoopStatus.h"

Fei4DataProcessor::Fei4DataProcessor(unsigned arg_hitDiscCfg) : DataProcessor(){
    input = NULL;
    hitDiscCfg = arg_hitDiscCfg;
    totCode = {{{1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 14, 0},
                  {2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 1, 0},
                  {3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 1, 0}}};
    

}

Fei4DataProcessor::~Fei4DataProcessor() {

}

void Fei4DataProcessor::init() {
    for(std::map<unsigned, ClipBoard<Fei4Data>* >::iterator it = outMap.begin(); it != outMap.end(); ++it) {
        activeChannels.push_back(it->first);
    }
}

void Fei4DataProcessor::process() {
    std::map<unsigned, unsigned> l1id;
    std::map<unsigned, unsigned> bcid;
    std::map<unsigned, unsigned> wordCount;
    std::map<unsigned, int> hits;
    
    unsigned badCnt = 0;
    for (unsigned i=0; i<activeChannels.size(); i++) {
        l1id[i] = 0;
        bcid[i] = 0;
        wordCount[i] = 0;
        hits[i] = 0;
    }
    
    unsigned dataCnt = 0;
    while(!input->empty()) {
        // Get data containers
        RawData *curIn = input->popData();
        if (curIn == NULL)
            continue;
        std::map<unsigned, Fei4Data*> curOut;
        std::map<unsigned, int> events;
        for (unsigned i=0; i<activeChannels.size(); i++) {
            curOut[i] = new Fei4Data();
            curOut[i]->lStat = curIn->stat;
            events[i] = 0;
        }

        // Process
        for (unsigned i=0; i<curIn->words; i++) {
            uint32_t value = curIn->buf[i];
            uint32_t header = ((value & 0x00FF0000) >> 16);
            unsigned channel = ((value & 0xFF000000) >> 24);
            wordCount[channel]++;
            if (value == 0xDEADBEEF)
                    std::cout << dataCnt << " [" << channel << "] Someting wrong: " << i << " " << curIn->words << " " << std::hex << value << " " << std::dec << std::endl;
            if (curOut[channel] == NULL) {
                std::cout << "# ERROR # " << __PRETTY_FUNCTION__ << " : Received data for channel " << channel << " but storage not initiliazed!" << std::endl;
            } else if (header == 0xe9) {
                // Delete empty events
                //if (events[channel] > 0 && hits[channel] == 0)
                    //curOut[channel]->delLastEvent();
                // Pixel Header
                l1id[channel] = (value & 0x7c00) >> 10;
                bcid[channel] = (value & 0x03FF);
                curOut[channel]->newEvent(l1id[channel], bcid[channel]);

                events[channel]++;
                //hits[channel] = 0;
            } else if (header == 0xef) {
                // Service Record
                unsigned code = (value & 0xFC00) >> 10;
                unsigned number = value & 0x03FF;
                curOut[channel]->serviceRecords[code]+=number;
            } else if (header == 0xea) {
                // Address Record
            } else if (header == 0xec) {
                // Value Record
            } else {
                uint16_t col = (value & 0xFE0000) >> 17;
                uint16_t row = (value & 0x01FF00) >> 8;
                uint8_t tot1 = (value & 0xF0) >> 4;
                uint8_t tot2 = (value & 0xF);
                if (events[channel] == 0 ) {
                    std::cout << i << " " << channel << " no header " << std::hex << value << std::dec << " " << col << " " << row << std::endl;
                    curOut[channel]->newEvent(l1id[channel], bcid[channel]);
                    events[channel]++;
                    //hits[channel] = 0;
                }
                if (col == 0 || row == 0 || col > 80 || row > 336) {
                    badCnt++;
                    std::cout << dataCnt << " [" << channel << "] Someting wrong: " << i << " " << curIn->words << " " << std::hex << value << " " << std::dec << std::endl;
                    if (i > 0) 
                        std::cout << "   " << std::hex << curIn->buf[i-1] << std::dec << std::endl;
                    if (i < (curIn->words-1))
                        std::cout << "   " << std::hex << curIn->buf[i+1] << std::dec << std::endl;
                } else {
                    if (totCode[hitDiscCfg][tot1] > 0) {
                        if (curOut[channel] != NULL)
                            curOut[channel]->curEvent->addHit(row, col, totCode[hitDiscCfg][tot1]);
                        hits[channel]++;
                    }
                    if (totCode[hitDiscCfg][tot1] > 0) {
                        if (curOut[channel] != NULL)
                            curOut[channel]->curEvent->addHit(row+1, col, totCode[hitDiscCfg][tot2]);
                        hits[channel]++;
                    }
                }
            }
        }
        for (unsigned i=0; i<activeChannels.size(); i++) {
            outMap[i]->pushData(curOut[i]);
        }
        //Cleanup
        delete curIn;
        dataCnt++;
    }
}
