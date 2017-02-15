/*
 * Authors: T. Heim <timon.heim@cern.ch>,
 * Date: 2014-May-17
 */

#include "Fe65p2GlobalFeedback.h"

Fe65p2GlobalFeedback::Fe65p2GlobalFeedback(Fe65p2GlobalReg Fe65p2GlobalCfg::*reg) {
    m_reg = reg;
    loopType = typeid(this);
    cur = 0;    
    m_done = false;
    max = 255;
}

void Fe65p2GlobalFeedback::feedbackBinary(unsigned channel, double sign, bool last) {
    // Calculate new step and value
    int val = (values[channel]+(localStep[channel]*sign));
    if (val < 0) val = 0;
    values[channel] = val;
    localStep[channel]  = localStep[channel]/2;
    doneMap[channel] |= last;

    if (localStep[channel] == 1) {
        doneMap[channel] = true;
    }

    // Unlock the mutex to let the scan proceed
    keeper->mutexMap[channel].unlock();

}

void Fe65p2GlobalFeedback::feedback(unsigned channel, double sign, bool last) {
    // Calculate new step and val
    std::cout << "in feedback" << std::endl;
    if (sign != oldSign[channel]) {
        oldSign[channel] = 0;
        localStep[channel] = localStep[channel]/2;
    }
    int val = (values[channel]+(localStep[channel]*sign));
    if (val > (int)max) val = max;
    if (val < 0) val = 0;
    values[channel] = val;
    doneMap[channel] |= last;

    if (localStep[channel] == 1) {
        doneMap[channel] = true;
    }

    //if (val < 120 && localStep[channel] > 4)
    //    localStep[channel] = 4;

    // Unlock the mutex to let the scan proceed
    std::cout << "unlock" << std::endl;
    keeper->mutexMap[channel].unlock();

}

void Fe65p2GlobalFeedback::init() {
    m_done = false;
    cur = 0;
    // TODO loop over active chips
    unsigned ch = 0; // hardcoded
    localStep[ch] = step;
    values[ch] = max;
    oldSign[ch] = -1;
    doneMap[ch] = false;
    
    keeper->globalFe<Fe65p2>()->setValue(m_reg, (uint16_t) values[ch]);
    keeper->globalFe<Fe65p2>()->configureGlobal();
}

void Fe65p2GlobalFeedback::end() {
    unsigned ch = 0;
    std:: cout << "---> Final parameter of Fe " << ch << " is " << values[ch] << std::endl;
}

void Fe65p2GlobalFeedback::execPart1() {
    g_stat->set(this, cur);
    unsigned ch = 0;
    keeper->mutexMap[ch].try_lock();
    m_done = doneMap[ch];
    
}

void Fe65p2GlobalFeedback::execPart2() {
    unsigned ch = 0; // TODO hardcoded on ch0
    keeper->mutexMap[ch].lock();
    std::cout << "---> Received Feedback for Fe " << ch << " with value " << values[ch] << std::endl;
    
    dynamic_cast<Fe65p2*>(keeper->feList[ch])->setValue(m_reg, (uint16_t) values[ch]);
    keeper->globalFe<Fe65p2>()->setValue(m_reg, (uint16_t) values[ch]);
    keeper->globalFe<Fe65p2>()->configureGlobal();
    
    cur++;
}

