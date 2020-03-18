/*
 * Authors: T. Heim <timon.heim@cern.ch>
 * Date: 2014-Oct-20
 */

#ifndef LOOPSTATUS_H
#define LOOPSTATUS_H

#include <vector>
#include <map>

class LoopActionBase;

/// Store of position within the scan hierarchy
class LoopStatus {
        std::vector<unsigned> statVec;

    public:
        /// Use to explicitly mark no location
        static LoopStatus empty() { return LoopStatus({}); }

        LoopStatus(const std::vector<unsigned> &&vec) : statVec(vec) {}
        size_t size() const { return statVec.size(); }
        unsigned get(unsigned i) const { return statVec[i]; }

        bool operator==(const LoopStatus &l){
                return statVec == l.statVec;
        }
};

/// Where we are in the scan engine (this one gets updated)
class LoopStatusMaster {
    public:
        LoopStatusMaster() {}

        LoopStatusMaster(const LoopStatusMaster &l) = delete;
        LoopStatus& operator=(const LoopStatus &l) = delete;

        /// Used to take a snapshot of where we are
        LoopStatus record() const { return LoopStatus{std::move(statVec)}; }

        void init(unsigned i) {
            statVec.resize(i);
            loopVec.resize(i);
        }
        void addLoop(unsigned i, LoopActionBase *l) {
            statMap[l] = &statVec[i];
            loopVec[i] = l;
        }
        
        void set(unsigned i, unsigned v) {statVec[i] = v;}
        void set(LoopActionBase *l, unsigned v) {*(statMap[l]) = v;}
        unsigned get(unsigned i) const {return statVec[i];}
        LoopActionBase* getPointer(unsigned i) const {return loopVec[i];}
        unsigned get(LoopActionBase *l) {return *(statMap[l]);}
        unsigned size() const {return statVec.size();}

        const std::map<LoopActionBase*, unsigned*> &getMap() const {return statMap;}
        const std::vector<unsigned> &getVector() const {return statVec;}
        const std::vector<LoopActionBase*> &getPointer() const {return loopVec;}
    private:
        /// Location within scan loop hierarchy
        std::vector<unsigned> statVec;

        /// Map from loop action pointers to location in master position
        std::map<LoopActionBase*, unsigned*> statMap;
        /// List of loop action pointers
        std::vector<LoopActionBase*> loopVec;
};

#endif
