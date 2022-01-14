#ifndef STARCOUNTERLOOP_H
#define STARCOUNTERLOOP_H

// #################################
// # Author:
// # Email:
// # Project: Yarr
// # Description: CounterLoop ABCStar
// # Comment:
// ################################

#include "LoopActionBase.h"
#include "StdTriggerAction.h"
#include "StarChips.h"

class StarCounterLoop: public LoopActionBase, public StdTriggerAction {
    public:
        StarCounterLoop();

        void setTrigDelay(uint32_t delay) {m_trigDelay = delay;}
        uint32_t getTrigDelay() {return m_trigDelay;}

        void setTrigFreq(double freq) {m_trigFreq = freq;}
        double getTrigFreq() {return m_trigFreq;}

        void setTrigTime(double time){m_trigTime = time;}
        double getTrigTime(){return m_trigTime;}

        void setNoInject();
        void setTrigWord();

        void writeConfig(json &config);
        void loadConfig(const json &config);

    private:
        uint32_t m_trigDelay;
        double m_trigFreq;
        double m_trigTime;

        // How many words of pattern buffer to use
        uint32_t m_trigWordLength;
        // This matches the pattern buffer in TxCore
        std::array<uint32_t, 32> m_trigWord;

        bool m_noInject;

        void init();
        void end();
        void execPart1();
        void execPart2();
};

#endif

