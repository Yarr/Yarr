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

class RawData {
    public:
        RawData(uint32_t arg_adr, uint32_t *arg_buf, size_t arg_words) {
            adr = arg_adr;
            buf = arg_buf;
            words = arg_words;
        }
        ~RawData() {
            delete buf;
        }
        uint32_t adr;
        uint32_t *buf;
        size_t words;
};

#endif
