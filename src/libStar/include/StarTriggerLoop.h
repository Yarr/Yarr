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

#define TRIG_CMD 0xe8000000
#define CAL_CMD  0x00000164

class StarTriggerLoop: public LoopActionBase {
    public:
        StarTriggerLoop();
        
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

        int bitlen(uint64_t cmd) { //@@@
       	 int length = 0;
       	 while (cmd) {
       		 cmd >>= 1;
       		 length += 1;
       	 }
       	 return length;
        }

    private:
        unsigned m_trigCnt;
        unsigned m_trigDelay;
        double m_trigFreq;
        double m_trigTime;
        uint32_t m_trigWord[4];
        uint32_t m_trigWordLength;
//        std::vector<uint32_t> m_trigWord;
        bool m_noInject; // TODO implement in init
        bool m_extTrigger; // TODO implement in init

        bool isInner;

        void init();
        void end();
        void execPart1();
        void execPart2();

        std::string interleaveL0CMD(uint64_t cmd, unsigned l0_delay=0);
        void prepareL0Trigger(); //broadcast
        void prepareL1Trigger(unsigned l0ID = 0); //broadcast
};

#endif

