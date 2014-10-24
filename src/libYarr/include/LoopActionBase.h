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

#include "Fei4.h"
#include "TxCore.h"
#include "RxCore.h"
#include "LoopStatus.h"

using std::shared_ptr;

class LoopActionBase {
    public:
        LoopActionBase();

        void setSplitData(bool v=true);
        bool split();

        void setVerbose(bool v=true);
        void setup(LoopStatus *stat, Fei4 *fe, TxCore *tx, RxCore *rx);
        void setNext(shared_ptr<LoopActionBase>& ptr);
        void execute();
        
    protected:
        virtual void init() {}
        virtual void end() {}
        virtual void execPart1() {}
        virtual void execPart2() {}
        virtual bool done();

        bool m_done;
        bool verbose;
        bool splitData;

        LoopStatus *g_stat;
        Fei4 *g_fe;
        TxCore *g_tx;
        RxCore *g_rx;

        std::type_index loopType;
    private:
        void execStep();
        void run();

        shared_ptr<LoopActionBase> m_inner;
};

#endif
