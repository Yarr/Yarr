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
class LoopStatusMaster;
class RxCore;
class TxCore;

class LoopActionBase : public LoopActionBaseInfo {
    public:
        explicit LoopActionBase(LoopStyle s);
        virtual ~LoopActionBase() = default;

        void setup(LoopStatusMaster *stat, Bookkeeper *k);
        void setNext(std::shared_ptr<LoopActionBase>& ptr);
        void execute();

        std::type_index type() {
            return loopType;
        }

        void setMin(unsigned v);
        void setMax(unsigned v);
        void setStep(unsigned v);

        virtual void loadConfig(const json &config) {}
        virtual void writeConfig(json &config) {}
		
    protected:
        virtual void init() {}
        virtual void end() {}
        virtual void execPart1() {}
        virtual void execPart2() {}
        virtual bool done();

        bool m_done;

        double progress;

        LoopStatusMaster *g_stat;
        FrontEnd *g_fe;
        TxCore *g_tx;
        RxCore *g_rx;
		Bookkeeper *keeper;

        std::type_index loopType;

    private:
        void execStep();
        void run();

        std::shared_ptr<LoopActionBase> m_inner;
};

#endif
