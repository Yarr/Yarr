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
#include <vector>
#include <string>

class Fei4Hit {
    public:
        Fei4Hit(unsigned arg_row, unsigned arg_col, unsigned arg_tot) {
            row = arg_row;
            col = arg_col;
            tot = arg_tot;
        }
        ~Fei4Hit(){};

        unsigned row;
        unsigned col;
        unsigned tot;
};

class Fei4Event {
    public:
        Fei4Event(unsigned arg_l1id, unsigned arg_bcid) {
            l1id = arg_l1id;
            bcid = arg_bcid;
            nHits = 0;
        }
        ~Fei4Event() {
            while(!hits.empty()) {
                Fei4Hit *tmp = hits.back();
                hits.pop_back();
                delete tmp;
            }
        }

        void addHit(unsigned arg_row, unsigned arg_col, unsigned arg_tot) {
            hits.push_back(new Fei4Hit(arg_row, arg_col, arg_tot));
            nHits++;
        }

        unsigned l1id;
        unsigned bcid;
        unsigned nHits;
        std::deque<Fei4Hit*> hits;
};

class Fei4Data {
    public:
        static const unsigned numServiceRecords = 32;
        Fei4Data() {
            curEvent = NULL;
            for(unsigned i=0; i<numServiceRecords; i++)
                serviceRecords.push_back(0);
        }
        ~Fei4Data() {
            while(!events.empty()) {
                Fei4Event *tmp = events.back();
                events.pop_back();
                delete tmp;
            }
        }

        void newEvent(unsigned arg_l1id, unsigned arg_bcid) {
            curEvent = new Fei4Event(arg_l1id, arg_bcid);
            events.push_back(curEvent);
        }

        void toFile(std::string filename);


        Fei4Event *curEvent;
        std::deque<Fei4Event*> events;
        std::vector<int> serviceRecords;
};



#endif
