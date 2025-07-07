#ifndef YARR_EVENT_DATA_H
#define YARR_EVENT_DATA_H

// #################################
// # Author: Daniel Joseph Antrim
// # Email: daniel.joseph.antrim AT cern.ch
// # Project: YARR
// # Description: Generic front-end data structure
// #################################

// std/stl
#include <string>
#include <vector>
#include <iostream>
#include <unistd.h> // this is needed for magical unknown reasons
#include <stdint.h>

// YARR
#include "EventDataBase.h"
#include "LoopStatus.h"

struct FrontEndHit {
    uint16_t col : 16;
    uint16_t row : 16;
    union {
        uint16_t tot : 16;
        struct {
            uint16_t ptot : 11;
            uint8_t ptoa : 5;
        };
    };
};

inline bool operator==(const FrontEndHit &lhs, const FrontEndHit &rhs)
{
    return lhs.col == rhs.col
         && lhs.row == rhs.row
         && lhs.tot == rhs.tot;
}

inline bool operator!=(const FrontEndHit &lhs, const FrontEndHit &rhs)
{
    return ! (lhs == rhs);
}

class FrontEndEvent {

    public:
        FrontEndEvent() {
            tag = 0;
            l1id = 0;
            bcid = 0;
            nHits = 0;
        }
        FrontEndEvent(unsigned arg_tag, unsigned arg_l1id, unsigned arg_bcid) {
            tag = arg_tag;
            l1id = arg_l1id;
            bcid = arg_bcid;
            nHits = 0;
        }
        ~FrontEndEvent() = default;
        void addEvent(const FrontEndEvent& event) {
            hits.insert(hits.end(), event.hits.begin(), event.hits.end());
            nHits += event.nHits;
        }
        void addHit(FrontEndHit hit) {
            hits.push_back(hit);
            nHits++;
        }
        void addHit(unsigned arg_row, unsigned arg_col, unsigned arg_timing) {
            hits.push_back(FrontEndHit{static_cast<uint16_t>(arg_col), static_cast<uint16_t>(arg_row), static_cast<uint16_t>(arg_timing)});
            nHits++;
        }

        void toFileBinary(std::fstream &handle) const;
        void fromFileBinary(std::fstream &handle);

        uint16_t l1id;
        uint16_t bcid;
        uint32_t tag;
        uint16_t nHits;
        std::vector<FrontEndHit> hits;
}; // class Event


class FrontEndData : public EventDataBase {

    public:

        FrontEndData()=default;
        FrontEndData(const LoopStatus& l) : lStat(l) {}
        ~FrontEndData() override = default;

        void delLastEvent() {
            events.pop_back();
            curEvent = &events.back();
        }

        void newEvent(unsigned arg_tag, unsigned arg_l1id, unsigned arg_bcid) {
            events.emplace_back(FrontEndEvent(arg_tag, arg_l1id, arg_bcid));
            curEvent = &events.back();
        }
        void toFile(std::string filename);
        
        FrontEndEvent* curEvent;
        LoopStatus lStat;
        std::vector<FrontEndEvent> events;
        std::vector<int> serviceRecords;

}; // class Data


#endif // YARR_EVENT_DATA_H
