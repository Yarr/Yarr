// #################################
// # Author: Timon Heim
// # Email: timon.heim at cern.ch
// # Project: Yarr
// # Description: FE65P2 Data Processor
// # Comment: 
// ################################

#include "Fe65p2DataProcessor.h"
#include "EventData.h"
#include "AllProcessors.h"

#include <iostream>

bool fe65p2_proc_registered =
    StdDict::registerDataProcessor("FE65P2", []() { return std::unique_ptr<DataProcessor>(new Fe65p2DataProcessor());});


Fe65p2DataProcessor::Fe65p2DataProcessor() : DataProcessor() {
    input = NULL;
}

Fe65p2DataProcessor::~Fe65p2DataProcessor() = default;

void Fe65p2DataProcessor::init() {
}

void Fe65p2DataProcessor::run() {
  std::cout << __PRETTY_FUNCTION__ << std::endl;
  thread_ptr.reset( new std::thread(&Fe65p2DataProcessor::process, this) );
}

void Fe65p2DataProcessor::join() {
  thread_ptr->join();
}


void Fe65p2DataProcessor::process() {
  while(true) {
    input->waitNotEmptyOrDone();
    
    process_core();
    
    if( input->isDone() ) {
      break;
    }
  }
  
  process_core();
}

void Fe65p2DataProcessor::process_core() {
    unsigned badCnt = 0;
    tag = 0;
    l1id = 0;
    bcid = 0;
    wordCount = 0;
    hits = 0;

    unsigned dataCnt = 0;
    while(!input->empty()) {
        // Get data containers
        std::unique_ptr<RawDataContainer> curInV = input->popData();
        if (curInV == nullptr)
            continue;

        // Create Output Container
        std::unique_ptr<FrontEndData> curOut;
        curOut.reset(new FrontEndData(curInV->stat));
        int events = 0;

        unsigned size = curInV->size();
        //if (size == 0)
        //std::cout << "Empty!" << std::endl;
        for(unsigned c=0; c<size; c++) {
            std::shared_ptr<RawData> curIn = curInV->data[c];
            // Process
            unsigned words = curIn->getSize();
            for (unsigned i=0; i<words; i++) {
                uint32_t value = curIn->get(i);
                unsigned channel = ((value & 0xFC000000) >> 26);
                unsigned type = ((value &0x03000000) >> 24);
                if (type == 0x1) {
                    tag = unsigned(value & 0x00FFFFFF);
                } else {
                    wordCount++;
                    if (__builtin_expect((value == 0xDEADBEEF), 0)) {
                        std::cout << "# ERROR # " << dataCnt << " [" << channel << "] Someting wrong: " << i << " " << words << " " << std::hex << value << " " << std::dec << std::endl;
                    } else if (__builtin_expect((curOut == nullptr), 0)) {
                        std::cout << "# ERROR # " << __PRETTY_FUNCTION__ << " : Received data for channel " << channel << " but storage not initiliazed!" << std::endl;
                    } else if ((value & 0x00800000) == 0x00800000) {
                        // BCID
                        if ((int)(value & 0x007FFFFF) - (int)(bcid) > 1) {
                            l1id++; // Iterate L1id when not consecutive bcid
                        }
                        bcid = (value & 0x007FFFFF);
                        curOut->newEvent(tag, l1id, bcid);
                        events++;
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

                            if (events == 0 ) {
                                std::cout << "# ERROR # " << channel << " no header in data fragment!" << std::endl;
                                curOut->newEvent(0xDEADBEEF, l1id, bcid);
                                events++;
                                //hits = 0;
                            }
                            if (__builtin_expect((real_col == 0 || real_row0 == 0 || real_col > 64 || real_row0 > 64), 0)) {
                                badCnt++;
                                std::cout << dataCnt << " [" << channel << "] Someting wrong: " << i << " " << words << " " << std::hex << value << " " << std::dec << std::endl;
                            } else {
                                if (tot0 != 15) {
                                    curOut->curEvent->addHit(real_row0, real_col, tot0);
                                    //std::cout << " hit!" << std::endl;
                                    hits++;
                                }
                                if (tot1 != 15) {
                                    curOut->curEvent->addHit(real_row1, real_col, tot1);
                                    hits++;
                                }
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

