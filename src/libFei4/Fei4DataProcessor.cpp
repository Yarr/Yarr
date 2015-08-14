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
    for(std::map<unsigned, ClipBoard<Fei4Data> >::iterator it = outMap->begin(); it != outMap->end(); ++it) {
        activeChannels.push_back(it->first);
    }
}

void Fei4DataProcessor::process() {
    unsigned badCnt = 0;
    for (unsigned i=0; i<activeChannels.size(); i++) {
        l1id[activeChannels[i]] = 0;
        bcid[activeChannels[i]] = 0;
        wordCount[activeChannels[i]] = 0;
        hits[activeChannels[i]] = 0;
    }

    unsigned dataCnt = 0;
    while(!input->empty()) {
        // Get data containers
        RawDataContainer *curInV = input->popData();
        if (curInV == NULL)
            continue;
        
        // Create Output Container
        std::map<unsigned, Fei4Data*> curOut;
        std::map<unsigned, int> events;
        for (unsigned i=0; i<activeChannels.size(); i++) {
            curOut[activeChannels[i]] = new Fei4Data();
            curOut[activeChannels[i]]->lStat = curInV->stat;
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
                unsigned channel = ((value & 0xFF000000) >> 24);
                wordCount[channel]++;
                if (__builtin_expect((value == 0xDEADBEEF), 0)) {
                    std::cout << "# ERROR # " << dataCnt << " [" << channel << "] Someting wrong: " << i << " " << curIn->words << " " << std::hex << value << " " << std::dec << std::endl;
                } else if (__builtin_expect((curOut[channel] == NULL), 0)) {
                    std::cout << "# ERROR # " << __PRETTY_FUNCTION__ << " : Received data for channel " << channel << " but storage not initiliazed!" << std::endl;
                } else if (header == 0xe9) {
                    // Pixel Header
                    l1id[channel] = (value & 0x7c00) >> 10;
                    bcid[channel] = (value & 0x03FF);
                    curOut[channel]->newEvent(l1id[channel], bcid[channel]);

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
                        std::cout << "# ERROR # " << channel << " no header in data fragment!" << std::endl;
                        curOut[channel]->newEvent(l1id[channel], bcid[channel]);
                        events[channel]++;
                        //hits[channel] = 0;
                    }
                    if (__builtin_expect((col == 0 || row == 0 || col > 80 || row > 336), 0)) {
                        badCnt++;
                        std::cout << dataCnt << " [" << channel << "] Someting wrong: " << i << " " << curIn->words << " " << std::hex << value << " " << std::dec << std::endl;
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
                if (badCnt > 10)
                    break;
            }
            delete curIn;
        }
        for (unsigned i=0; i<activeChannels.size(); i++) {
            outMap->at(activeChannels[i]).pushData(curOut[activeChannels[i]]);
        }
        //Cleanup
        delete curInV;
        dataCnt++;
    }
}
