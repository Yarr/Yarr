/*
 * Authors: T. Heim <timon.heim@cern.ch>
 * Date: 2013-Oct-22
 */

#ifndef LOOPENGINE_H
#define LOOPENGINE_H

#include "EngineTBase.h"
#include "LoopActionBase.h"
#include "TxCore.h"
#include "RxCore.h"

#include "Bookkeeper.h"

typedef EngineTBase< std::vector< std::shared_ptr<LoopActionBase> > > Engine;

class LoopEngine : public Engine {
    public:
        LoopEngine(Bookkeeper *k);
        ~LoopEngine();
        
        void addAction(Engine::element_value_type el);
        
        void init();
        void execute();
        void end();

    private:
        Engine::loop_list_type m_list;
        LoopStatus stat;
        Bookkeeper *g_bk;
};

#endif
