/*
 * Authors: T. Heim <timon.heim@cern.ch>
 * Date: 2014-Oct-20
 */

#ifndef LOOPSTATUS_H
#define LOOPSTATUS_H

#include <algorithm>
#include <array>
#include <bitset>
#include <map>
#include <stdexcept>
#include <vector>

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
        std::bitset<MAX_LOOP_SIZE> enabledVec; ///< true if the loop is enabled, only used in uniqueID and toString
                                               ///< this allows us to ignore certain loops in the ID and string representation
                                               ///< while still keeping the full loop information in the object.
                                               ///< initialized to all true in constructor

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
            enabledVec.set(); // all enabled by default
        }

        LoopStatus(const std::vector<unsigned> &vec, const std::vector<LoopStyle> &vec_s) : statCount(vec.size())
        {
            if(statCount > MAX_LOOP_SIZE) {
                throw std::logic_error("Too many loops");
            }
            std::copy(vec.begin(), vec.end(), statVec.begin());
            std::copy(vec_s.begin(), vec_s.end(), styleVec.begin());
            enabledVec.set(); // all enabled by default
        }

        size_t size() const { return statCount; }
        unsigned get(unsigned i) const { return statVec[i]; }
        LoopStyle getStyle(unsigned i) const { return styleVec[i]; }

        /// @brief Enable only the specified loops for UID and toString representation (disable all others)
        /// @param loopsToEnable Vector of loop indices to enable
        void setEnabledLoops(const std::vector<size_t> &loopsToEnable) {
            enabledVec.reset();
            for (size_t i = 0; i < loopsToEnable.size(); ++i) {
                if(loopsToEnable[i] < MAX_LOOP_SIZE) {
                    enabledVec[loopsToEnable[i]] = true;
                }
            }
        }

        /// @brief Disable the specified loops for UID and toString representation (enable all others)
        /// @param loopsToDisable Vector of loop indices to disable
        void setDisabledLoops(const std::vector<size_t> &loopsToDisable) {
            enabledVec.set();
            for (size_t i = 0; i < loopsToDisable.size(); ++i) {
                if(loopsToDisable[i] < MAX_LOOP_SIZE) {
                    enabledVec[loopsToDisable[i]] = false;
                }
            }
        }

        using UID = std::array<unsigned, MAX_LOOP_SIZE>;

        /// @brief Create a unique ID from the current loop status
        /// e.g. to be used as the key for a std::map in an analysis.
        /// Output is dependent on which loops are enabled via setEnabledLoops or setDisabledLoops.
        /// @return Unique loop status ID
        UID uniqueID() const {
            UID uid;
            for (size_t i = 0; i < statCount; i++) {
                if (enabledVec[i]) {
                    uid[i] = statVec[i];
                } else {
                    uid[i] = 0;
                }
            }

            // explicitly zero-initialize the rest
            for (size_t i = statCount; i < MAX_LOOP_SIZE; i++) {
                uid[i] = 0;
            }

            return uid;
        }

        /// @brief Create a dash-separated loop status string
        /// e.g. to be added to the name of a histogram
        /// Output is dependent on which loops are enabled via setEnabledLoops or setDisabledLoops.
        /// @return Loop status string
        std::string toString() const {
            std::string result;
            for (size_t i = 0; i < statCount; ++i) {
                if (enabledVec[i]) {
                    result += std::to_string(statVec[i]);
                    result += "-";
                }
            }
            if (!result.empty()) {
                result.pop_back();  // Remove trailing dash
            }
            return result;
        }

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
