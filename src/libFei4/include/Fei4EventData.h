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

class Fei4Hit {
    public:
        Fei4Hit(int arg_bcid, int arg_row, int arg_col, int arg_tot) {
            bcid = arg_bcid;
            row = arg_row;
            col = arg_col;
            tot = arg_tot;
        }
        
        int bcid;
        int row;
        int col;
        int tot;
};

class Fei4Event {
    public:
        Fei4Event(int arg_l1id) {
            l1id = arg_l1id;
        }

        void addHit(int arg_bcid, int arg_row, int arg_col, int arg_tot) {
            hits.push_back(Fei4Hit(arg_bcid, arg_row, arg_col, arg_tot));
        }
        
        int l1id;
        std::deque<Fei4Hit> hits;
};

class Fei4Data {
    public:
         static const unsigned numServiceRecords = 32;
         Fei4Data() {
            for(unsigned i=0; i<numServiceRecords; i++)
                serviceRecords.push_back(0);
         }

        void newEvent(int arg_l1id) {
            events.push_back(Fei4Event(arg_l1id));
        }

        void toFile(std::string filename);
        
        std::deque<Fei4Event> events;
        std::vector<int> serviceRecords;
};



#endif
