#ifndef RAWDATA_H
#define RAWDATA_H

// #################################
// # Author: Timon Heim
// # Email: timon.heim at cern.ch
// # Project: Yarr
// # Description: Raw Data Container
// # Comment: Not really fancy
// ################################

#include <cstdint>
#include <memory>

#include "LoopStatus.h"

struct RawData {
    RawData(uint32_t arg_adr, uint32_t *arg_buf, unsigned arg_words) :
            adr(arg_adr),  buf(arg_buf), words(arg_words) {}
    ~RawData() {
        delete[] buf;
    }
    uint32_t adr;
    uint32_t *buf;
    unsigned words;
};

class RawDataContainer {
    public:
        RawDataContainer(LoopStatus &&s) : stat(s) {}
        ~RawDataContainer() {
        }

        void add(std::shared_ptr<RawData> arg_data) {
            data.emplace_back(arg_data);
        }

        unsigned size() const {
            return data.size();
        }

        std::vector<std::shared_ptr<RawData>> data;
        LoopStatus stat;
};

#endif
