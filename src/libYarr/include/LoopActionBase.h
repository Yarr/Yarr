/*
 * Authors: K. Potamianos <karolos.potamianos@cern.ch>,
 *          T. Heim <timon.heim@cern.ch>
 * Date: 2013-Oct-22
 */

#ifndef LOOPACTIONBASE_H
#define LOOPACTIONBASE_H

#include <memory>
#include <vector>
#include <map>

#include "Fei4.h"
#include "TxCore.h"
#include "RxCore.h"

using std::shared_ptr;

class LoopActionBase;

class LoopStatus {
    public:
        void addLoop(LoopActionBase *loop) {
            statVec.push_back(0);
            statMap[loop] = &statVec.back();
        }
        
        void set(unsigned i, double v) {statVec[i] = v;}
        void set(LoopActionBase *l, double v) {*statMap[l] = v;}
        unsigned get(unsigned i) {return statVec[i];}
        unsigned get(LoopActionBase *l) {return *statMap[l];}

    private:
        std::map<LoopActionBase*, unsigned*> statMap;    
        std::vector<unsigned> statVec;
};

class LoopActionBase {
    public:
        LoopActionBase();

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

        LoopStatus *g_stat;
        Fei4 *g_fe;
        TxCore *g_tx;
        RxCore *g_rx;

    private:
        void execStep();
        void run();

        shared_ptr<LoopActionBase> m_inner;
};

#endif
