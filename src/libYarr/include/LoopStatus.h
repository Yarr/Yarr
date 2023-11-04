/*
 * Authors: T. Heim <timon.heim@cern.ch>
 * Date: 2014-Oct-20
 */

#ifndef LOOPSTATUS_H
#define LOOPSTATUS_H

#include <iostream>
#include <vector>
#include <map>

class LoopActionBase;

enum LoopStyle {
    /// Data loop generates data (connected to FE output)
    LOOP_STYLE_DATA,
    /// Loop over many triggers (implements StdTriggerAction)
    LOOP_STYLE_TRIGGER,
    /// Changes to mask, which are summed over in histogram
    LOOP_STYLE_MASK,
    /// Change to a paramter, different bin in analysis
    LOOP_STYLE_PARAMETER,
    /// Waits for feedback to pixel level from analysis
    LOOP_STYLE_PIXEL_FEEDBACK,
    /// Waits for coarse grained feedback from analysis
    LOOP_STYLE_GLOBAL_FEEDBACK,
    /// Loop action that has no other effect on scan
    LOOP_STYLE_NOP,
    /// Sentinel
    LOOP_STYLE_END
};

/// Store of position within the scan hierarchy
class LoopStatus {
   private:
        std::vector<unsigned> statVec;
        std::vector<LoopStyle> styleVec;

    public:
        LoopStatus()=default;

        LoopStatus(const std::vector<unsigned> &&vec, const std::vector<LoopStyle> &vec2) : statVec(vec), styleVec(vec2) {}
        size_t size() const { return statVec.size(); }
        unsigned get(unsigned i) const { return statVec[i]; }
        
        size_t styleSize() const { return styleVec.size(); }
        unsigned getStyle(unsigned i) const { return styleVec[i]; }

        bool operator==(const LoopStatus &l){
                return statVec == l.statVec;
        }

        bool is_end_of_iteration = true;
};

/// Where we are in the scan engine (this one gets updated)
class LoopStatusMaster {
    public:
        LoopStatusMaster() = default;

        LoopStatusMaster(const LoopStatusMaster &l) = delete;
        LoopStatus& operator=(const LoopStatus &l) = delete;

        /// Used to take a snapshot of where we are
        LoopStatus record() const { return LoopStatus{std::move(statVec), styleVec}; }

        void init(unsigned i) {
            statVec.resize(i);
            styleVec.resize(i);
            loopVec.resize(i);
        }
        void addLoop(unsigned i, LoopActionBase *l, LoopStyle loopStyle) {
            statMap[l] = &statVec[i];
            styleVec[i] = loopStyle;
            loopVec[i] = l;
        }
        
        void set(unsigned i, unsigned v) {statVec[i] = v;}
        void set(LoopActionBase *l, unsigned v) {
            *(statMap[l]) = v;
        }
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
        std::vector<LoopStyle> styleVec;

        /// Map from loop action pointers to location in master position
        std::map<LoopActionBase*, unsigned*> statMap;
        /// List of loop action pointers
        std::vector<LoopActionBase*> loopVec;
};

#endif
