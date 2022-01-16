/*
 * Authors: T. Heim <timon.heim@cern.ch>
 * Date: 2016-Mar-30
 */

#ifndef FE65P2TRIGGERLOOP_H
#define FE65P2TRIGGERLOOP_H

#include "LoopActionBase.h"
#include "TxCore.h"
#include "StdTriggerAction.h"
#include "Fe65p2.h"

class Fe65p2TriggerLoop : public LoopActionBase, public StdTriggerAction {
    public:
        Fe65p2TriggerLoop();

        // Overrides StdTriggerAction, but not virtual
        void setTrigCnt(uint32_t cnt);

        void setTrigFreq(double freq);
        double getTrigFreq() const;
        void setTrigTime(double time);
        double getTrigTime() const;

        void setNoInject();
        void setExtTrigger();

    private:
        unsigned m_trigDelay;
        double m_trigFreq;
        double m_trigTime;
        uint32_t m_trigWord[4];
        uint32_t m_trigWordLength;
        enum TRIG_CONF_VALUE m_trigMode;

        void init() override;
        void end() override;
        void execPart1() override;
        void execPart2() override;

};

#endif
