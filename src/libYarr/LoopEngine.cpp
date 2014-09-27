/*
 * Authors: T. Heim <timon.heim@cern.ch>
 * Date: 2013-Oct-22
 */

#include "LoopEngine.h"

// Our LoopEngine will take care of distributing the FeGrp to each loop item
LoopEngine::LoopEngine() {

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
    while(m_list.end() != it) {
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
