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
        unsigned getTrigDelay() const;
        void setTrigFreq(double freq);
        double getTrigFreq() const;
        void setTrigTime(double time);
        double getTrigTime() const;
        void setNoInject();
        void setTrigWord(uint32_t word[4]);
        void setNoWord();

        //void setIsInner(bool itis=true);
        //bool getIsInner();

        void writeConfig(json &config) override;
        void loadConfig(const json &config) override;
        
        uint32_t getExpEvents() override;
    private:
        unsigned m_trigDelay;
        float m_trigFreq;
        float m_trigTime;
        uint32_t m_trigWord[4];
        uint32_t m_trigWordLength;

        bool m_noInject; // TODO implement in init
        bool m_extTrigger; // TODO implement in init

        bool isInner;

        void init() override;
        void end() override;
        void execPart1() override;
        void execPart2() override;
};

#endif

