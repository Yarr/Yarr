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

class FrontEndCluster {
    public :
        FrontEndCluster() { nHits = 0; }
        ~FrontEndCluster() {}

        void addHit(FrontEndHit* hit) {
            hits.push_back(hit);
            nHits++;
        }
        unsigned getColLength() {
            int min = 99999;
            int max = -1;
            for (unsigned ii = 0; ii < hits.size(); ii++) {
                if ((int)hits.at(ii)->col > max)
                    max = hits.at(ii)->col;
                if ((int)hits.at(ii)->col < min)
                    min = hits.at(ii)->col;
            } // ii
            return (max-min+1);
        }

        unsigned getRowWidth() {
            int min = 99999;
            int max = -1;
            for (unsigned ii = 0; ii < hits.size(); ii++) {
                if ((int)hits.at(ii)->row > max)
                    max = hits.at(ii)->row;
                if ((int)hits.at(ii)->row < min)
                    min = hits.at(ii)->row;
            } // ii
            return (max-min+1);
        }

        unsigned nHits;
        std::vector<FrontEndHit*> hits;

}; // class FrontEndCluster

class FrontEndEvent {

    public:
        FrontEndEvent() {
            tag = 0;
            l1id = 0;
            bcid = 0;
            nHits = 0;
            nClusters = 0;
        }
        FrontEndEvent(unsigned arg_tag, unsigned arg_l1id, unsigned arg_bcid) {
            tag = arg_tag;
            l1id = arg_l1id;
            bcid = arg_bcid;
            nHits = 0;
            nClusters = 0;
        }
        ~FrontEndEvent() {};
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

        void doClustering();
        void toFileBinary(std::fstream &handle) const;
        void fromFileBinary(std::fstream &handle);

        uint16_t l1id;
        uint16_t bcid;
        uint16_t tag;
        uint16_t nHits;
        uint16_t nClusters;
        std::vector<FrontEndHit> hits;
        std::vector<FrontEndCluster> clusters;

}; // class Event


class FrontEndData : public EventDataBase {

    public:

        FrontEndData() : lStat(LoopStatus::empty()) {}
        FrontEndData(LoopStatus& l) : lStat(l) {
            for(unsigned ii = 0; ii < numServiceRecords; ii++) {
                serviceRecords.push_back(0);
            } // ii
        };
        ~FrontEndData() {};
        static const unsigned numServiceRecords = 32;

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
