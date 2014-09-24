#include "Fei4DataProcessor.h"

#include <iostream>

Fei4DataProcessor::Fei4DataProcessor() {
    input = NULL;
    output = NULL;
}

Fei4DataProcessor::~Fei4DataProcessor() {

}

void Fei4DataProcessor::process() {
    int oldl1id = 0;
    int oldbcid = 0;
    while(!input->empty()) {
        // Get data containers
        RawData *curIn = input->popData();
        Fei4Data *curOut = new Fei4Data();

        // Process
        int hits = 0;
        int events = 0;
        for (unsigned i=0; i<curIn->words; i++) {
            int l1id = -1;
            int bcid = 1;
            uint32_t value = curIn->buf[i];
            if ((value & 0x00FF0000) >> 16 == 0xe9) {
                // Pixel Header
                l1id = (value & 0x7c00) >> 10;
                bcid = (value & 0x03FF);
                curOut->newEvent(l1id);
                oldl1id = l1id;
                oldbcid = bcid;
                events++;
            } else if ((value & 0x00FF0000) >> 16 == 0xef) {
                // Service Record
                unsigned code = (value & 0xFC00) >> 10;
                unsigned number = value & 0x03FF;
                curOut->serviceRecords[code]+=number;
            } else {
                if (events == 0 ) {
                    std::cout << "Ripped event" << std::endl;
                    l1id = oldl1id;
                    bcid = oldbcid;
                    curOut->newEvent(l1id);
                    events++;
                }
                unsigned col = (value & 0xFE0000) >> 17;
                unsigned row = (value & 0x01FF00) >> 8;
                unsigned tot1 = (value & 0xF0) >> 4;
                unsigned tot2 = (value & 0xF);
                if (tot1 < 0xC) {
                    if (curOut != NULL)
                        curOut->curEvent->addHit(bcid, row, col, tot1);
                    hits++;
                }
                if (tot2 < 0xC) {
                    if (curOut != NULL)
                        curOut->curEvent->addHit(bcid, row+1, col, tot2);
                    hits++;
                }
            }
        }

        output->pushData(curOut);
        std::cout << "Total events = " << events << std::endl;
        std::cout << "Total hits = " << hits << std::endl;

        //Cleanup
        delete curIn;
    }
}
