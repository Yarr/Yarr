/*
 * Authors: T. Heim <timon.heim@cern.ch>,
 * Date: 2013-Oct-22
 */

#ifndef STDTRIGGERLOOP_H
#define STDTRIGGERLOOP_H

#include <LoopActionBase.h>

class StdTriggerLoop: public LoopActionBase {
    public:
        StdTriggerLoop();
        
        void setMaxTrigCnt(unsigned int cnt);
        unsigned int getMaxTrigCnt();
        void setTrigDelay(unsigned int delay);
        unsigned int getTrigDelay();

    private:
        unsigned int m_maxTrigCnt;
        unsigned int m_curTrigCnt;
        unsigned int m_trigDelay;

        void init();
        void end();
        void execPart1();
        void execPart2();
};

#endif

