/*
 * Authors: T. Heim <timon.heim@cern.ch>,
 * Date: 2013-Oct-22
 */

#ifndef FEI4TRIGGERLOOP_H
#define FEI4TRIGGERLOOP_H

#include "LoopActionBase.h"

#define TRIG_CMD 0xe8000000
#define CAL_CMD  0x00000164

class Fei4TriggerLoop: public LoopActionBase {
    public:
        Fei4TriggerLoop();
        
        void setTrigCnt(unsigned int cnt);
        unsigned getTrigCnt();
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
        unsigned m_trigCnt;
        unsigned m_trigDelay;
        double m_trigFreq;
        double m_trigTime;
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

