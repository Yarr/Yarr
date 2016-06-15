#ifndef RAWDATA_H
#define RAWDATA_H

// #################################
// # Author: Timon Heim
// # Email: timon.heim at cern.ch
// # Project: Yarr
// # Description: Raw Data Container
// # Comment: Not really fancy
// ################################

#include <stdint.h>

#include "LoopStatus.h"

class RawData {
    public:
        RawData(uint32_t arg_adr, uint32_t *arg_buf, unsigned arg_words);
        ~RawData();
        
        uint32_t adr;
        uint32_t *buf;
        unsigned words;
        LoopStatus stat;
};

class RawDataContainer {
    public:
        RawDataContainer(){}
        ~RawDataContainer() {
            for(unsigned int i=0; i<adr.size(); i++)
                delete[] buf[i];
        }

        void add(RawData *d) {
            adr.push_back(d->adr);
            buf.push_back(d->buf);
            words.push_back(d->words);
            delete d;
        }

        unsigned size() {
            return adr.size();
        }

        std::vector<uint32_t> adr;
        std::vector<uint32_t*> buf;
        std::vector<unsigned> words;
        LoopStatus stat;
};

#endif
