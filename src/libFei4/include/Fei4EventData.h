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
#include <unistd.h>

#include "LoopStatus.h"

struct Fei4Hit {
    unsigned col : 7;
    unsigned row : 9;
    unsigned tot : 5;
    unsigned unused : 11;
};

class Fei4Cluster {
    public:
        Fei4Cluster() {
            nHits = 0;
        }
        ~Fei4Cluster() {}

        void addHit(Fei4Hit* hit) {
            hits.push_back(hit);
            nHits++;
        }

        unsigned getColLength() {
            int min = 999999;
            int max = -1;
            for (unsigned i=0; i<hits.size(); i++) {
                if (hits[i]->col > max)
                    max = hits[i]->col;
                if (hits[i]->col < min)
                    min = hits[i]->col;
            }
            return max-min+1;
        }
        
        unsigned getRowWidth() {
            int min = 999999;
            int max = -1;
            for (unsigned i=0; i<hits.size(); i++) {
                if (hits[i]->row > max)
                    max = hits[i]->row;
                if (hits[i]->row < min)
                    min = hits[i]->row;
            }
            return max-min+1;
        }

        unsigned nHits;
        std::vector<Fei4Hit*> hits;
};

class Fei4Event {
    public:
        Fei4Event() {
            l1id = 0;
            bcid = 0;
            nHits = 0;
            nClusters = 0;
        }
        Fei4Event(unsigned arg_l1id, unsigned arg_bcid) {
            l1id = arg_l1id;
            bcid = arg_bcid;
            nHits = 0;
            nClusters = 0;
        }
        ~Fei4Event() {
            //while(!hits.empty()) {
                //Fei4Hit *tmp = hits.back();
                //hits.pop_back();
                //delete tmp;
            //}
        }

        void addHit(unsigned arg_row, unsigned arg_col, unsigned arg_tot) {
            struct Fei4Hit tmp = {arg_col, arg_row, arg_tot, 0};
            //tmp.col = arg_col;
            //tmp.row = arg_row;
            //tmp.tot = arg_tot;
            //tmp.unused = 0;
            hits.push_back(Fei4Hit(tmp));
            nHits++;
        }

        void doClustering();

        void toFileBinary(std::fstream &handle);
        void fromFileBinary(std::fstream &handle);

        uint16_t l1id;
        uint16_t bcid;
        uint16_t nHits;
        uint16_t nClusters;
        std::vector<Fei4Hit> hits;
        std::vector<Fei4Cluster> clusters;
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
