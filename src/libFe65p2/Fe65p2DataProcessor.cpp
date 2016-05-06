// #################################
// # Author: Timon Heim
// # Email: timon.heim at cern.ch
// # Project: Yarr
// # Description: FE65P2 Data Processor
// # Comment: 
// ################################

#include "Fe65p2DataProcessor.h"

Fe65p2DataProcessor::Fe65p2DataProcessor() : DataProcessor() {
    input = NULL;
}

Fe65p2DataProcessor::~Fe65p2DataProcessor() {
}

void Fe65p2DataProcessor::init() {
    for(std::map<unsigned, ClipBoard<Fei4Data> >::iterator it = outMap->begin(); it != outMap->end(); ++it) {
        activeChannels.push_back(it->first);
    }
}

void Fe65p2DataProcessor::process() {
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
                unsigned channel = ((value & 0xFF000000) >> 24);
                wordCount[channel]++;
                if (__builtin_expect((value == 0xDEADBEEF), 0)) {
                    std::cout << "# ERROR # " << dataCnt << " [" << channel << "] Someting wrong: " << i << " " << curIn->words << " " << std::hex << value << " " << std::dec << std::endl;
                } else if (__builtin_expect((curOut[channel] == NULL), 0)) {
                    std::cout << "# ERROR # " << __PRETTY_FUNCTION__ << " : Received data for channel " << channel << " but storage not initiliazed!" << std::endl;
                } else if ((value & 0x00800000) == 0x00800000) {
                    // BCID
                    if ((int)(value & 0x007FFFFF) - (int)(bcid[channel]) > 1) {
                        l1id[channel]++; // Iterate L1id when not consecutive bcid
                    }
                    bcid[channel] = (value & 0x007FFFFF);
                    curOut[channel]->newEvent(l1id[channel], bcid[channel]);
                    events[channel]++;
                } else {
                    unsigned col  = (value & 0x1e0000) >> 17;
                    unsigned row  = (value & 0x01F800) >> 11;
                    unsigned rowp = (value & 0x000400) >> 10;
                    unsigned tot0 = (value & 0x0000F0) >> 4;
                    unsigned tot1 = (value & 0x00000F) >> 0;

                    if ((tot0 != 15 || tot1 != 15) && (tot0 > 0 || tot1 > 0)) {
                        unsigned real_col = 0;
                        unsigned real_row0 = 0;
                        unsigned real_row1 = 0;
                        if (rowp == 1) {
                            real_col = (col*4) + ((row/32)*2) + 1;
                        } else {
                            real_col = (col*4) + ((row/32)*2) + 2;
                        }

                        if (row < 32) {
                            real_row1 = (row+1)*2;
                            real_row0 = (row+1)*2 - 1;
                        } else {
                            real_row1 = 64 - (row-32)*2;
                            real_row0 = 64 - (row-32)*2 - 1;
                        }

                        if (events[channel] == 0 ) {
                            std::cout << "# ERROR # " << channel << " no header in data fragment!" << std::endl;
                            curOut[channel]->newEvent(l1id[channel], bcid[channel]);
                            events[channel]++;
                            //hits[channel] = 0;
                        }
                        if (__builtin_expect((real_col == 0 || real_row0 == 0 || real_col > 64 || real_row0 > 64), 0)) {
                            badCnt++;
                            std::cout << dataCnt << " [" << channel << "] Someting wrong: " << i << " " << curIn->words << " " << std::hex << value << " " << std::dec << std::endl;
                        } else {
                            if (tot0 != 15) {
                                curOut[channel]->curEvent->addHit(real_row0, real_col, tot0);
                                //std::cout << " hit!" << std::endl;
                                hits[channel]++;
                            }
                            if (tot1 != 15) {
                                curOut[channel]->curEvent->addHit(real_row1, real_col, tot1);
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
            outMap->at(activeChannels[i]).pushData(curOut[activeChannels[i]]);
        }
        //Cleanup
        delete curInV;
        dataCnt++;
    }

}

