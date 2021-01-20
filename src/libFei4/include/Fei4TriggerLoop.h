/*
 * Authors: T. Heim <timon.heim@cern.ch>,
 * Date: 2013-Oct-22
 */

#ifndef FEI4TRIGGERLOOP_H
#define FEI4TRIGGERLOOP_H

#include "LoopActionBase.h"
#include "Fei4.h"
#include "StdTriggerAction.h"

#define TRIG_CMD 0xe8000000
#define CAL_CMD  0x00000164

class Fei4TriggerLoop: public LoopActionBase, public StdTriggerAction {
    public:
        Fei4TriggerLoop();
        
        void setTrigDelay(unsigned int delay);
        unsigned getTrigDelay();
        void setTrigFreq(double freq);
        double getTrigFreq();
        void setTrigTime(double time);
        double getTrigTime();
        void setNoInject();
        void setTrigWord(uint32_t word[4]);
        void setNoWord();

        //void setIsInner(bool itis=true);
        //bool getIsInner();

        void writeConfig(json &config);
        void loadConfig(json &config);
    private:
        unsigned m_trigDelay;
        float m_trigFreq;
        float m_trigTime;
        uint32_t m_trigWord[4];
        uint32_t m_trigWordLength;

        bool m_noInject; // TODO implement in init
        bool m_extTrigger; // TODO implement in init

        bool isInner;

        void init();
        void end();
        void execPart1();
        void execPart2();
};

#endif

