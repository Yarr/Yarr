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

class RawData {
    public:
        RawData(uint32_t arg_adr, unsigned arg_words) {
            adr = arg_adr;
            buf.reserve(arg_words);   
        }

        ~RawData()=default;

        inline uint32_t& getAdr() {
            return adr;
        }

        inline uint32_t* getBuf() {
            return buf.data();
        }

        inline size_t getSize() {
            return buf.size();
        }

        inline uint32_t& operator [](size_t i) {
            return buf[i];
        }

        inline uint32_t& at(size_t i) {
            return buf[i];
        }
        
    private:
        std::vector<uint32_t> buf;
        uint32_t adr;
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
