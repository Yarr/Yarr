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

/**
 * A block of raw data.
 */
class RawData {
    public:
        /** Associate data with channel */
        RawData(uint32_t arg_adr, unsigned arg_words) {
            adr = arg_adr;
            buf.resize(arg_words, 0);    
            buf.reserve(arg_words);   
        }

        // move ctor
        RawData(uint32_t arg_adr, std::vector<uint32_t> &&arg_buf) : adr(arg_adr) {
            buf = std::move(arg_buf);
        }

        // copy ctor
        RawData(uint32_t arg_adr, std::vector<uint32_t> const &arg_buf) : adr(arg_adr) {
            buf = arg_buf;
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
        /// The buffer
        std::vector<uint32_t> buf;
        /// Address of rx channel
        uint32_t adr;
};

using RawDataPtr = std::shared_ptr<RawData>;

/**
 * Array of RawData's. LoopStatus is common.
 */
class RawDataContainer {
    public:
        /** Create empty container */
        RawDataContainer(const LoopStatus &s) : stat(s) {}
        
        /** Destroy attached RawData. */
        ~RawDataContainer()=default;

        /** Add data to collection */
        inline void add(RawDataPtr arg_data) {
            data.push_back(arg_data);
        }

        /** Return size of collection */
        inline unsigned size() const {
            return data.size();
        }

        std::vector<RawDataPtr> data;
        LoopStatus stat;
};

#endif
