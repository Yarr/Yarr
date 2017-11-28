/*
 * Authors: T. Heim <timon.heim@cern.ch>
 * Date: 2014-Oct-20
 */

#ifndef LOOPSTATUS_H
#define LOOPSTATUS_H

#include <array>
#include <iostream>
#include <stdexcept>
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
    /// Trigger loop that waits for feedback from analysis
    LOOP_STYLE_TRIGGER_FEEDBACK,
    /// Loop action that has no other effect on scan
    LOOP_STYLE_NOP,
    /// Sentinel
    LOOP_STYLE_END
};

/**
 * Representation of current position in a scan loop.
 *
 * Also used as a description of the loop hierarchy.
 */
class LoopStatus {
    private:
        static const size_t MAX_LOOP_SIZE = 8;

        size_t statCount{};
        std::array<unsigned, MAX_LOOP_SIZE> statVec;
        std::array<LoopStyle, MAX_LOOP_SIZE> styleVec;

    public:
        /** Create LoopStatus */
        LoopStatus()=default;

        LoopStatus(std::initializer_list<unsigned> vec, std::initializer_list<LoopStyle> vec_s) : statCount(vec.size())
        {
            if(statCount > MAX_LOOP_SIZE) {
                throw std::logic_error("Too many loops");
            }
            std::copy(vec.begin(), vec.end(), statVec.begin());
            std::copy(vec_s.begin(), vec_s.end(), styleVec.begin());
        }
        LoopStatus(const std::vector<unsigned> &vec, const std::vector<LoopStyle> &vec_s) : statCount(vec.size())
        {
            if(statCount > MAX_LOOP_SIZE) {
                throw std::logic_error("Too many loops");
            }
          std::copy(vec.begin(), vec.end(), statVec.begin());
          std::copy(vec_s.begin(), vec_s.end(), styleVec.begin());
        }
        size_t size() const { return statCount; }
        unsigned get(unsigned i) const { return statVec[i]; }
        
        unsigned getStyle(unsigned i) const { return styleVec[i]; }

        /** Compare with another LoopStatus */
        bool operator==(const LoopStatus &l){
                return statVec == l.statVec;
        }

        bool is_end_of_iteration = true;
};

/**
 * Representation of current location in the scan engine.
 *
 * This gets updated during the scan and LoopStatus is recorded to tag bits of data.
 */
class LoopStatusMaster {
    public:
        LoopStatusMaster() = default;

        LoopStatusMaster(const LoopStatusMaster &l) = delete;
        /** Copy LoopStatus */
        LoopStatus& operator=(const LoopStatus &l) = delete;

        /// Used to take a snapshot of where we are
        LoopStatus record() const { return LoopStatus(statVec, styleVec); }

        /** Initialise */
        void init(unsigned i) {
            statVec.resize(i);
            styleVec.resize(i);
            loopVec.resize(i);
        }
        /**
         * Add a layer.
         *
         * @param i The layer index.
         * @param l The layer object.
         * @param loopStyle The type of the layer.
         */
        void addLoop(unsigned i, LoopActionBase *l, LoopStyle loopStyle) {
            statMap[l] = &statVec[i];
            styleVec[i] = loopStyle;
            loopVec[i] = l;
        }
        
        /** Set position at layer */
        void set(unsigned i, unsigned v) {statVec[i] = v;}
        /** Set position at layer given by type */
        void set(LoopActionBase *l, unsigned v) {
            *(statMap[l]) = v;
        }
        /** Retrieve stats for layer */
        unsigned get(unsigned i) const {return statVec[i];}
        /** Retrieve implementer for scan layer */
        LoopActionBase* getPointer(unsigned i) const {return loopVec[i];}
        /** Retrieve position using type */
        unsigned get(LoopActionBase *l) {return *(statMap[l]);}
        /** Return number of loops */
        unsigned size() const {return statVec.size();}

        /** Get map from type to stat */
        const std::map<LoopActionBase*, unsigned*> &getMap() const {return statMap;}
        /** Get array of loop positions */
        const std::vector<unsigned> &getVector() const {return statVec;}
        /** Get map from layer number to loop object */
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
