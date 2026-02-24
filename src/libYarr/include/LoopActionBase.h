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
#include <array>

#include "LoopActionBaseInfo.h"

#include "storage.hpp"

class Bookkeeper;
class FrontEnd;
template<typename T>
class ClipBoard;
class HistogramBase;
class LoopStatusMaster;
class RxCore;
class TxCore;

/**
 * Implementation of a layer of a nested loop.
 *
 * LoopEngine effectively executes:
 *
 * init()
 * for v in values:
 *  execPart1()
 *  run inner loop
 *  execPart2()
 * end()
 * 
 */
class LoopActionBase : public LoopActionBaseInfo {
    public:
        explicit LoopActionBase(LoopStyle s);
        virtual ~LoopActionBase() = default;

        /**
         * Attach to information about scan engine.
         *
         * @param stat Stores the current loop position.
         * @param k Reference to bookkeeper.
         */
        void setup(LoopStatusMaster *stat, Bookkeeper *k);
        
        /**
         * Executed once after all loops have finished.
         */
        virtual void closeOut() {}

        /// Set the inner loop
        void setNext(std::shared_ptr<LoopActionBase>& ptr);

        /// Run this loop
        void execute();

        /// Type of class implementing the loop
        std::type_index type() {
            return loopType;
        }

        /// Set minimum value
        void setMin(unsigned v);
        /// Set maximum value
        void setMax(unsigned v);
        /// Set step value
        void setStep(unsigned v);

        /// Configure this loop.
        virtual void loadConfig(const json &config) {}
        /// Dump configuration of this loop.
        virtual void writeConfig(json &config) {}

    protected:
        /// Do at start of the loop
        virtual void init() {}
        /// Do at end of the loop
        virtual void end() {}
        /// Do at start of each step
        virtual void execPart1() {}
        /// Do at end of each step
        virtual void execPart2() {}
        /// Is loop complete
        virtual bool done();

        bool m_done;

        double progress;

        LoopStatusMaster *g_stat;
        FrontEnd *g_fe;
        TxCore *g_tx;
        RxCore *g_rx;
		Bookkeeper *keeper;

        std::type_index loopType;

        ClipBoard<HistogramBase> *loopHistos;

    private:
        void execStep();
        void run();

        std::shared_ptr<LoopActionBase> m_inner;
};

#endif
