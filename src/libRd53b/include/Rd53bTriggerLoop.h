#ifndef RD53BTRIGGERLOOP_H
#define RD53BTRIGGERLOOP_H

// #################################
// # Author: Timon Heim
// # Email: timon.heim at cern.ch
// # Project: Yarr
// # Description: Trigger Loop for RD53B
// # Date: 07/2020
// ################################

#include "LoopActionBase.h"
#include "Rd53b.h"
#include "Rd53bCmd.h"

class Rd53bTriggerLoop : public LoopActionBase {
    public:
        Rd53bTriggerLoop();
        
        uint32_t getTrigCnt() {return m_trigCnt;}
        void setTrigCnt(uint32_t cnt) {m_trigCnt = cnt;}
        void setTrigTime(double time) {m_trigTime = time;}
        void setTrigFreq(double freq) {m_trigFreq = freq;}
        void setTrigDelay(uint32_t delay);
        void setEdgeMode(uint32_t duration);
        void setNoInject();

        void writeConfig(json &config);
        void loadConfig(json &config);

    private:
        uint32_t m_trigCnt;
        uint32_t m_trigDelay;
        float m_trigTime;
        float m_trigFreq;
        std::array<uint32_t, 32> m_trigWord;
        uint32_t m_trigWordLength;
        bool m_noInject;
        bool m_edgeMode;
        bool m_extTrig;
        uint32_t m_edgeDuration;
        uint32_t m_pulseDuration;
        uint32_t m_trigMultiplier;

        void init();
        void end();
        void execPart1();
        void execPart2();
};

#endif
