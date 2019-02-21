// #################################
// # Author: Timon Heim
// # Email: timon.heim at cern.ch
// # Project: Yarr
// # Description: RD53A Data Processor
// # Date: Apr 2018
// ################################

#include "Rd53aDataProcessor.h"
#include "AllProcessors.h"

bool rd53a_proc_registered =
    StdDict::registerDataProcessor("RD53A", []() { return std::unique_ptr<DataProcessor>(new Rd53aDataProcessor());});


Rd53aDataProcessor::Rd53aDataProcessor() {
    verbose = true;
    m_input = NULL;
}

Rd53aDataProcessor::~Rd53aDataProcessor() {

}

void Rd53aDataProcessor::init() {
    if (verbose)
        std::cout << __PRETTY_FUNCTION__ << std::endl;

    for (auto &it : *m_outMap) {
        activeChannels.push_back(it.first);
    }
    scanDone = false;
}

void Rd53aDataProcessor::run() {
    if (verbose)
        std::cout << __PRETTY_FUNCTION__ << std::endl;
    unsigned int numThreads = std::thread::hardware_concurrency();
    if (numThreads > 4) numThreads = 4;
    for (unsigned i=0; i<numThreads; i++) {
        thread_ptrs.emplace_back(new std::thread(&Rd53aDataProcessor::process, this));
        std::cout << "  -> Processor thread #" << i << " started!" << std::endl;
    }
}

void Rd53aDataProcessor::join() {
    for( auto& thread : thread_ptrs ) {
        if( thread->joinable() ) thread->join();
    }
}

void Rd53aDataProcessor::process() {
    while(true) {
        std::unique_lock<std::mutex> lk(mtx);
        m_input->cv.wait( lk, [&] { return scanDone || !m_input->empty(); } );

        process_core();
        for (unsigned i=0; i<activeChannels.size(); i++) {
            m_outMap->at(activeChannels[i]).cv.notify_all(); // notification to the downstream
        }
        // TODO the timing on these seems sensitive
        std::this_thread::sleep_for(std::chrono::microseconds(200));
        if( scanDone ) {
            std::this_thread::sleep_for(std::chrono::microseconds(200));
            process_core(); // this line is needed if the data comes in before scanDone is changed.
            for (unsigned i=0; i<activeChannels.size(); i++) {
                m_outMap->at(activeChannels[i]).cv.notify_all(); // notification to the downstream
            }
            break;
        }
    }

    process_core();
}

void Rd53aDataProcessor::process_core() {
    // TODO put data from channels back into input, so other processors can use it
    for (auto &i : activeChannels) {
        tag[i] = 666;
        l1id[i] = 666;
        bcid[i] = 666;
        wordCount[i] = 0;
        hits[i] = 0;
    }

    unsigned dataCnt = 0;
    while(!m_input->empty()) {
        // Get data containers
        auto curInV = m_input->popData();
        if (curInV == nullptr)
            continue;

        // Create Output Container
        std::map<unsigned, std::unique_ptr<Fei4Data>> curOut;
        std::map<unsigned, int> events;
        for (unsigned i=0; i<activeChannels.size(); i++) {
            curOut[activeChannels[i]].reset(new Fei4Data());
            curOut[activeChannels[i]]->lStat = curInV->stat;
            events[activeChannels[i]] = 0;
        }

        unsigned size = curInV->size();
        for(unsigned c=0; c<size; c++) {
            RawData *curIn = new RawData(curInV->adr[c], curInV->buf[c], curInV->words[c]);
            // Process
            unsigned words = curIn->words;
            dataCnt += words;
            for (unsigned i=0; i<words; i++) {
                // Decode content
                // TODO this needs review, can't deal with user-k data
                uint32_t data = curIn->buf[i];
                unsigned channel = activeChannels[(i/2)%activeChannels.size()];
                //std::cout << "[" << i << "]\t\t[" << channel << "] = 0x" << std::hex << data << std::dec << std::endl;
                if (__builtin_expect(((data & 0xFFFF0000) != 0xFFFF0000 ), 1)) {
                    if ((data >> 25) & 0x1) { // is header
                        l1id[channel] = 0x1F & (data >> 20);
                        tag[channel] = 0x1F & (data >> 15);
                        bcid[channel] = 0x7FFF & data;
                        // Create new event
                        curOut[channel]->newEvent(tag[channel], l1id[channel], bcid[channel]);
                        events[channel]++;
                        //std::cout << "[Header] : L1ID(" << l1id[channel] 
                        //    << ") TAG(" << tag[channel] << ") BCID(" << bcid[channel] << ")" << std::endl;
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
                        //std::cout << "[Data] : COL(" << core_col << ") ROW(" << core_row  << ") Region(" << region
                        //    << ") TOT(" << tot3 << "," << tot2 << "," << tot1 << "," << tot0 
                        //    << ") RAW(0x" << std::hex << data << std::dec << ")" << std::endl;

                        if (__builtin_expect((pix_col < Rd53a::n_Col && pix_row < Rd53a::n_Row), 1)) {
                            // Check if there is already an event
                            if (events[channel] == 0) {
                                //std::cout << "# WARNING # " << channel << " no header in data fragment!" << std::endl;
                                curOut[channel]->newEvent(tag[channel], l1id[channel], bcid[channel]);
                                events[channel]++;
                            }
                            // TODO Make decision on pixel address start 0,0 or 1,1
                            pix_row++;
                            pix_col++;
                            if (tot0 != 0xF) {
                                curOut[channel]->curEvent->addHit(pix_row, pix_col, tot0+1);
                                hits[channel]++;
                            }
                            if (tot1 != 0xF) {
                                curOut[channel]->curEvent->addHit(pix_row, pix_col+1, tot1+1);
                                hits[channel]++;
                            }
                            if (tot2 != 0xF) {
                                curOut[channel]->curEvent->addHit(pix_row, pix_col+2, tot2+1);
                                hits[channel]++;
                            }
                            if (tot3 != 0xF) {
                                curOut[channel]->curEvent->addHit(pix_row, pix_col+3, tot3+1);
                                hits[channel]++;
                            }
                        } else {
                            std::cout << dataCnt << " [" << channel << "] Received data not valid: [" << i << "," << curIn->words << "] = 0x" << std::hex << data << " " << std::dec << std::endl;
                        }

                    }
                }
            }
            delete curIn;
        }

        // Push data out
        for (unsigned i=0; i<activeChannels.size(); i++) {
            if (events[activeChannels[i]] > 0) {
                m_outMap->at(activeChannels[i]).pushData(std::move(curOut[activeChannels[i]]));
            } else {
                // Maybe wait for end of method instead of deleting here?
                curOut[activeChannels[i]].reset();
            }

        }
        //Cleanup
    }

}
