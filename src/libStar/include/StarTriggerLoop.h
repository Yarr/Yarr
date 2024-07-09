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
#include "StdTriggerAction.h"
#include "StarChips.h"
#include "StarSeqGenerator.h"

/**
   Loop action for Star triggers.

   Trigger loop with implementation of charge injection for Strips front-end.
 */
class StarTriggerLoop: public LoopActionBase, public StdTriggerAction {
    public:
        StarTriggerLoop();

        void setTrigDelay(uint32_t delay) {m_trigDelay = delay;}
        uint32_t getTrigDelay() const {return m_trigDelay;}

        void setTrigFreq(double freq) {m_trigFreq = freq;}
        double getTrigFreq() const {return m_trigFreq;}

        void setTrigTime(double time){m_trigTime = time;}
        double getTrigTime() const{return m_trigTime;}

        void setDigital(bool digital){m_digital = digital;}
        bool getDigital() const{return m_digital;}

        void setNoInject();
        void setTrigWord();
        void setTrigWordFromFile();

        void writeConfig(json &config) override;
        void loadConfig(const json &config) override;

    private:
        uint32_t m_trigDelay;
        double m_trigFreq;
        double m_trigTime;
        uint32_t m_cmdCnt{0}; // number of times to send the command sequence

        std::vector<uint32_t> m_trigWord;

        bool m_noInject;
        bool m_digital;

        std::string m_fpathSeq;
        StarSeqGenerator m_seqGen{false};

        void init() override;
        void end() override;
        void execPart1() override;
        void execPart2() override;
};

#endif

