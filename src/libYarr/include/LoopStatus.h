/*
 * Authors: T. Heim <timon.heim@cern.ch>
 * Date: 2014-Oct-20
 */

#ifndef LOOPSTATUS_H
#define LOOPSTATUS_H

#include <memory>
#include <vector>
#include <map>

/// Structure of a scan loop
struct LoopInfo {
        std::map<void*, unsigned*> statMap;
        std::vector<void*> loopVec;
};

/// Where we are in the scan engine
class LoopStatus {
    public:
        LoopStatus()
          : statInfo(new LoopInfo)
        {}

        LoopStatus(const LoopStatus &l)
          : statInfo(l.statInfo) {
            statVec = l.getVector();
        }

        LoopStatus& operator=(const LoopStatus &l) {
            statInfo = l.statInfo;
            statVec = l.getVector();
            return *this;
        }

        bool operator==(const LoopStatus &l){
            return statVec == l.statVec;
        }

        void init(unsigned i) {
            statVec.resize(i);
            statInfo->loopVec.resize(i);
        }
        void addLoop(unsigned i, void *loop) {
            statInfo->statMap[loop] = &statVec[i];
            statInfo->loopVec[i] = loop;
        }
        
        void set(unsigned i, unsigned v) {statVec[i] = v;}
        void set(void *l, unsigned v) {*(statInfo->statMap[l]) = v;}
        unsigned get(unsigned i) const {return statVec[i];}
        void* getPointer(unsigned i) const {return statInfo->loopVec[i];}
        unsigned get(void *l) {return *(statInfo->statMap[l]);}
        unsigned size() const {return statVec.size();}

        const std::map<void*, unsigned*> getMap() const {return statInfo->statMap;}
        const std::vector<unsigned> getVector() const {return statVec;}
        const std::vector<void*> getPointer() const {return statInfo->loopVec;}
    private:
        /// Location within scan loop hierarchy
        std::vector<unsigned> statVec;

        /// Information used to lookup types of scan loops
        std::shared_ptr<LoopInfo> statInfo;
};

#endif
