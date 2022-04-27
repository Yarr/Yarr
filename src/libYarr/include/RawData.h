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
            buf.resize(arg_words, 0);    
            buf.reserve(arg_words);   
        }

        RawData(uint32_t arg_adr, std::vector<uint32_t> &arg_buf) {
            buf = std::move(arg_buf);
        }

        virtual ~RawData()=default;

        virtual inline void resize(unsigned arg_words) {
            buf.resize(arg_words);
            buf.reserve(arg_words);   
        }

        virtual inline uint32_t& getAdr() {
            return adr;
        }

        virtual inline uint32_t* getBuf() {
            return buf.data();
        }

        virtual inline unsigned getSize() const {
            return buf.size();
        }

        virtual inline uint32_t& operator [](size_t i) {
            return buf[i];
        }

        virtual inline uint32_t& get(size_t i) {
            return buf[i];
        }
        
    protected:
        std::vector<uint32_t> buf;
        uint32_t adr;
};

using RawDataPtr = std::shared_ptr<RawData>;

class RawDataContainer {
    public:
        RawDataContainer(LoopStatus &&s) : stat(s) {}
        
        ~RawDataContainer()=default;

        inline void add(std::shared_ptr<RawData> arg_data) {
            data.push_back(arg_data);
        }

        inline unsigned size() const {
            return data.size();
        }

        std::vector<std::shared_ptr<RawData>> data;
        LoopStatus stat;
};

#endif
