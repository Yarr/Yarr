/*
 * Authors: T. Heim <timon.heim@cern.ch>
 * Date: 2014-Oct-20
 */

#ifndef LOOPSTATUS_H
#define LOOPSTATUS_H

#include <iostream>
#include <vector>
#include <map>

class LoopStatus {
    public:
        LoopStatus() {}
        LoopStatus(const LoopStatus &l) {
            statMap = l.getMap();
            statVec = l.getVector();
        }

        LoopStatus& operator=(const LoopStatus &l) {
            statMap = l.getMap();
            statVec = l.getVector();
            return *this;
        }

        bool operator==(const LoopStatus &l){
            if (l.size() != this->size()) {
                return false;
            } else {
                for (unsigned i=0; i<l.size(); i++) {
                    if (l.get(i) != this->get(i)) {
                        return false;
                    }
                }
            }
            return true;
        }

        void init(unsigned i) {
            statVec.resize(i);
        }
        void addLoop(unsigned i, void *loop) {
            statMap[loop] = &statVec[i];
        }
        
        void set(unsigned i, unsigned v) {statVec[i] = v;}
        void set(void *l, unsigned v) {*(statMap[l]) = v;}
        unsigned get(unsigned i) const {return statVec[i];}
        unsigned get(void *l) {return *(statMap[l]);}
        unsigned size() const {return statVec.size();}

        std::map<void*, unsigned*> getMap() const {return statMap;}
        std::vector<unsigned> getVector() const {return statVec;}
    private:
        std::map<void*, unsigned*> statMap;    
        std::vector<unsigned> statVec;
};

#endif
