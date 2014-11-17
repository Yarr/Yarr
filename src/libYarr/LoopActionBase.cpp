/*
 * Authors: K. Potamianos <karolos.potamianos@cern.ch>,
 *          T. Heim <timon.heim@cern.ch>
 * Date: 2013-Oct-22
 */

#include "LoopActionBase.h"
#include <iostream>

LoopActionBase::LoopActionBase() : loopType(typeid(void)){
    g_fe = NULL;
    g_tx = NULL;
    g_rx = NULL;
    g_stat = NULL;
    verbose = false;
    m_done = false;
}

void LoopActionBase::setVerbose(bool v) {
    if (v)
        std::cout << __PRETTY_FUNCTION__ << " : Enabling debug output" << std::endl;
    verbose = v;
}

void LoopActionBase::setup(LoopStatus *stat, Fei4 *fe, TxCore *tx, RxCore *rx) {
    if (verbose)
        std::cout << __PRETTY_FUNCTION__ << std::endl;
    g_stat = stat;
    g_fe = fe;
    g_tx = tx;
    g_rx = rx;
}

void LoopActionBase::setNext(shared_ptr<LoopActionBase>& ptr) {
    m_inner = ptr;
}

void LoopActionBase::execute() {
    this->run();
}

bool LoopActionBase::done() {
    return m_done;
}

void LoopActionBase::execStep() {
    this->execPart1();
    
    if (m_inner) m_inner->execute();
#ifdef DEBUG
    else std::cout << "*** There's no other loop inside of me :)" << std::endl;
#endif
    
    this->execPart2();
}

void LoopActionBase::run() {
    this->init();
#ifdef DEBUG
    // Limit the loop to 1
    while(!this->done()) { this->execStep(); m_done = true; } 
#else
    while(!this->done()) this->execStep();
#endif
    this->end();

}

unsigned LoopActionBase::getMin() {
    return min;
}

unsigned LoopActionBase::getMax() {
    return max;
}

unsigned LoopActionBase::getStep() {
    return step;
}

void LoopActionBase::setMax(unsigned v) {
    max = v;
}

void LoopActionBase::setMin(unsigned v) {
    min = v;
}

void LoopActionBase::setStep(unsigned v) {
    step = v;
}
