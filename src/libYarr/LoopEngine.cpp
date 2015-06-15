/*
 * Authors: T. Heim <timon.heim@cern.ch>
 * Date: 2013-Oct-22
 */

#include "LoopEngine.h"

// Our LoopEngine will take care of distributing the global Fe to each loop item
LoopEngine::LoopEngine(Fei4 *fe, TxCore *tx, RxCore *rx) {
    g_fe = fe;
    g_tx = tx;
    g_rx = rx;
}

LoopEngine::LoopEngine(Bookkeeper *k) {
    g_fe = k->g_fe;
    g_tx = k->tx;
    g_rx = k->rx;
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
