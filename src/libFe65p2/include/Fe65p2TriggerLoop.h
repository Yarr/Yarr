/*
 * Authors: T. Heim <timon.heim@cern.ch>
 * Date: 2016-Mar-30
 */

#ifndef FE65P2TRIGGERLOOP_H
#define FE65P2TRIGGERLOOP_H

#include "LoopActionBase.h"
#include "TxCore.h"
#include "Fe65p2.h"

class Fe65p2TriggerLoop : public LoopActionBase {
    public:
        Fe65p2TriggerLoop();
        
        void setTrigCnt(unsigned int cnt);
        unsigned getTrigCnt();
        void setTrigFreq(double freq);
        double getTrigFreq();
        void setTrigTime(double time);
        double getTrigTime();

        void setNoInject();
        void setExtTrigger();

    private:
        unsigned m_trigCnt;
        unsigned m_trigDelay;
        double m_trigFreq;
        double m_trigTime;
        uint32_t m_trigWord[4];
        uint32_t m_trigWordLength;
        enum TRIG_CONF_VALUE m_trigMode;

        void init();
        void end();
        void execPart1();
        void execPart2();

};

#endif
