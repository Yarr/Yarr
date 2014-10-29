#ifndef FEI4EVENTDATA_H
#define FEI4EVENTDATA_H

// #################################
// # Author: Timon Heim
// # Email: timon.heim at cern.ch
// # Project: Yarr
// # Description: Container for FeI4 data
// # Comment: Splits events by L1ID
// ################################

#include <deque>
#include <list>
#include <vector>
#include <string>

#include "LoopStatus.h"

struct Fei4Hit {
    unsigned col : 7;
    unsigned row : 9;
    unsigned tot : 5;
    unsigned unused : 11;
};

class Fei4Event {
    public:
        Fei4Event() {
            l1id = 0;
            bcid = 0;
            nHits = 0;
        }
        Fei4Event(unsigned arg_l1id, unsigned arg_bcid) {
            l1id = arg_l1id;
            bcid = arg_bcid;
            nHits = 0;
        }
        ~Fei4Event() {
            //while(!hits.empty()) {
                //Fei4Hit *tmp = hits.back();
                //hits.pop_back();
                //delete tmp;
            //}
        }

        void addHit(unsigned arg_row, unsigned arg_col, unsigned arg_tot) {
            struct Fei4Hit tmp;
            tmp.col = arg_col;
            tmp.row = arg_row;
            tmp.tot = arg_tot;
            hits.push_back(Fei4Hit(tmp));
            nHits++;
        }

        uint16_t l1id;
        uint16_t bcid;
        uint16_t nHits;
        std::vector<Fei4Hit> hits;
};

class Fei4Data {
    public:
        static const unsigned numServiceRecords = 32;
        Fei4Data() {
            //curEvent = NULL;
            for(unsigned i=0; i<numServiceRecords; i++)
                serviceRecords.push_back(0);
        }
        ~Fei4Data() {
            //while(!events.empty()) {
            //    Fei4Event *tmp = events.back();
            //    events.pop_back();
            //     delete tmp;
            //}
        }

        void newEvent(unsigned arg_l1id, unsigned arg_bcid) {
            events.push_back(Fei4Event(arg_l1id, arg_bcid));
            curEvent = &events.back();
        }

        void delLastEvent() {
            //delete curEvent;
            events.pop_back();
            curEvent = &events.back();
        }

        void toFile(std::string filename);


        Fei4Event *curEvent;

        LoopStatus lStat;
        std::list<Fei4Event> events;
        std::vector<int> serviceRecords;
};



#endif
