/*
 * Authors: K. Potamianos <karolos.potamianos@cern.ch>,
 *          T. Heim <timon.heim@cern.ch>
 * Date: 2013-Oct-22
 */

#include "LoopActionBase.h"

#include "logging.h"

namespace {
    auto llog = logging::make_log("LoopActionBase");
}

LoopActionBase::LoopActionBase(LoopStyle l)
  : LoopActionBaseInfo(l),
    loopType(typeid(void))
{
    g_fe = nullptr;
    g_tx = nullptr;
    g_rx = nullptr;
    g_stat = nullptr;
    m_done = false;
}

void LoopActionBase::setup(LoopStatusMaster *stat, Bookkeeper *k) {
    SPDLOG_LOGGER_DEBUG(llog, "");
    g_stat = stat;
    g_fe = k->g_fe;
    g_tx = k->tx;
    g_rx = k->rx;
	keeper = k;
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
    
    this->execPart2();
}

void LoopActionBase::run() {
    this->init();
    while(!this->done()) this->execStep();
    this->end();
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
