#ifndef STARTRIGGERLOOP_H
#define STARTRIGGERLOOP_H

// #################################
// # Author:
// # Email:
// # Project: Yarr
// # Description: TriggerLoop ABCStar
// # Comment:
// ################################

#include "LoopActionBase.h"
#include "StarChips.h"



class StarTriggerLoop: public LoopActionBase {
    public:
        StarTriggerLoop();
        
        void setTrigCnt(uint32_t cnt) {m_trigCnt = cnt;}
        uint32_t getTrigCnt(){return m_trigCnt;};

        void setTrigDelay(uint32_t delay);
        uint32_t getTrigDelay() {return m_trigDelay;}

        void setTrigFreq(double freq) {m_trigFreq = freq;}
        double getTrigFreq() {return m_trigFreq;}

        void setTrigTime(double time){m_trigTime = time;}
        double getTrigTime(){return m_trigTime;}

        void setNoInject();
        void setTrigWord();

        void writeConfig(json &config);
        void loadConfig(json &config);

        int bitlen(uint64_t cmd) { //@@@
       	 int length = 0;
       	 while (cmd) {
       		 cmd >>= 1;
       		 length += 1;
       	 }
       	 return length;
        }




    private:
        uint32_t m_trigCnt;
        uint32_t m_trigDelay;
        double m_trigFreq;
        double m_trigTime;

        uint32_t m_trigWordLength;
        std::array<uint32_t, 32> m_trigWord;
        bool m_noInject;
        bool m_extTrigger;
        bool isInner;

        void init();
        void end();
        void execPart1();
        void execPart2();

};

#endif

