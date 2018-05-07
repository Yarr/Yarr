/*
 * Authors: T. Heim <timon.heim@cern.ch>
 * Date: 2013-Oct-22
 */

#include "LoopEngine.h"

// Our LoopEngine will take care of distributing the global Fe to each loop item
LoopEngine::LoopEngine(Bookkeeper *k) {
    g_bk = k;
}

LoopEngine::~LoopEngine() {
}

// Add an item/loop to the engine
void LoopEngine::addAction(Engine::element_value_type el){
    m_list.push_back(el);
}

// Iniitialization step, needed before execution
void LoopEngine::init() {
    Engine::loop_list_type::iterator it = m_list.begin();
   
    stat.init(m_list.size());
    
    unsigned i = 0;
    while(m_list.end() != it) {
        stat.addLoop(i, (*it).get());
        i++;
        (*it)->setup(&stat, g_bk);
        ++it;
    }
}

// Execution of items/loops
void LoopEngine::execute() {
    Engine::execute(m_list);
}

// What has to be done after execution
void LoopEngine::end() {

}
