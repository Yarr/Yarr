/*
 * Authors: K. Potamianos <karolos.potamianos@cern.ch>,
 *          T. Heim <timon.heim@cern.ch>
 * Date: 2013-Oct-22
 */

#ifndef LOOPACTIONBASE_H
#define LOOPACTIONBASE_H

#include <memory>
#include <typeinfo>
#include <typeindex>
#include <string>

#include "TxCore.h"
#include "RxCore.h"
#include "LoopStatus.h"
#include "Bookkeeper.h"


#include "storage.hpp"

using std::shared_ptr;

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

class LoopActionBase {
    public:
        explicit LoopActionBase(LoopStyle s);
        virtual ~LoopActionBase() {}

        void setup(LoopStatusMaster *stat, Bookkeeper *k);
        void setNext(shared_ptr<LoopActionBase>& ptr);
        void execute();

        std::type_index type() {
            return loopType;
        }

        bool isParameterLoop() {
            return m_style == LOOP_STYLE_PARAMETER;
        }

        bool isMaskLoop() {
            return m_style == LOOP_STYLE_MASK;
        }

        bool isDataLoop() {
            return m_style == LOOP_STYLE_DATA;
        }

        bool isTriggerLoop() {
            return m_style == LOOP_STYLE_TRIGGER;
        }

        bool isPixelFeedbackLoop() {
            return m_style == LOOP_STYLE_PIXEL_FEEDBACK;
        }

        bool isGlobalFeedbackLoop() {
            return m_style == LOOP_STYLE_GLOBAL_FEEDBACK;
        }

        unsigned getMin();
        unsigned getMax();
        unsigned getStep();
        void setMin(unsigned v);
        void setMax(unsigned v);
        void setStep(unsigned v);

        virtual void loadConfig(json &config) {}
        virtual void writeConfig(json &config) {}

    protected:
        virtual void init() {}
        virtual void end() {}
        virtual void execPart1() {}
        virtual void execPart2() {}
        virtual bool done();

        bool m_done;

        int min;
        int max;
        unsigned step;

        //minMultiple, maxMultiple, and stepMultiple used by Rd53aTriggerLoop, which can take multiple parameter values for concurrent stepping.
        //These vectors contain any additional parameters to step. The first parameter in the list is put in min, max, and step (above).
        std::vector<int> minMultiple;
        std::vector<int> maxMultiple;
        std::vector<int> stepMultiple;

        double progress;

        LoopStatusMaster *g_stat;
        FrontEnd *g_fe;
        TxCore *g_tx;
        RxCore *g_rx;
		Bookkeeper *keeper;
		std::map<unsigned, bool> doneMap;

        LoopStyle m_style;
        std::type_index loopType;

    private:
        void execStep();
        void run();

        shared_ptr<LoopActionBase> m_inner;
};

#endif
