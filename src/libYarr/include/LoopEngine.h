/*
 * Authors: T. Heim <timon.heim@cern.ch>
 * Date: 2013-Oct-22
 */

#ifndef LOOPENGINE_H
#define LOOPENGINE_H

#include "EngineTBase.h"

#include <memory>

#include "LoopStatus.h"

class Bookkeeper;
class LoopActionBase;

typedef EngineTBase< std::vector< std::shared_ptr<LoopActionBase> > > Engine;

/**
 * Top level engine, link all the loop levels together.
 */
class LoopEngine : public Engine {
    public:
        /** Create engine */
        LoopEngine(Bookkeeper *k);
        /** Destroy engine */
        ~LoopEngine();
        
        /** Add loop action to this engine */
        void addAction(Engine::element_value_type el);
        
        /** Initialise all the layers of the scan */
        void init();
        /** Run the scan */
        void execute();
        /** Called at the end of the scan */
        void end();

    private:
        Engine::loop_list_type m_list;
        LoopStatusMaster stat;
        Bookkeeper *g_bk;
};

#endif
