#include "Fei4DataProcessor.h"

#include <iostream>

Fei4DataProcessor::Fei4DataProcessor() {
    input = NULL;
    output = NULL;
}

Fei4DataProcessor::~Fei4DataProcessor() {

}

void Fei4DataProcessor::process() {
    while(!input->empty()) {
        // Get data containers
        RawData *curIn = NULL;
        input->popData(curIn);
        Fei4Data *curOut = new Fei4Data();

        // Process
        for (unsigned i=0; i<curIn->words; i++) {
            int l1id = -1;
            int bcid = 1;
            int hits = 0;
            uint32_t value = curIn->buf[i];
            if ((value & 0x00FF0000) >> 16 == 0xe9) {
                // Pixel Header
                if (l1id) {
                    std::cout << "New Event: " << l1id << " with " << hits << " hits!" << std::endl;
                }
                l1id = (value & 0x7c00) >> 10;
                bcid = (value & 0x03FF);
                curOut->newEvent(l1id);
                hits = 0;
            } else if ((value & 0x00FF0000) >> 16 == 0xef) {
                // Service Record
                unsigned code = (value & 0xFC00) >> 10;
                unsigned number = value & 0x03FF;
                curOut->serviceRecords[code]+=number;
            } else {
                unsigned col = (value & 0xFE0000) >> 17;
                unsigned row = (value & 0x01FF00) >> 8;
                unsigned tot1 = (value & 0xF0) >> 4;
                unsigned tot2 = (value & 0xF);
                if (tot1 > 0) {
                    curOut->events.back().addHit(bcid, col, row, tot1);
                    hits++;
                }
                if (tot2 > 0) {
                    curOut->events.back().addHit(bcid, col, row+1, tot2);
                    hits++;
                }
            }
        }

        //Cleanup
        delete curIn;
    }
}
