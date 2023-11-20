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
#include "StdTriggerAction.h"
#include "Rd53b.h"
#include "Rd53bCmd.h"

class Rd53bTriggerLoop : public LoopActionBase, public StdTriggerAction {
    public:
        Rd53bTriggerLoop();
        
        void setTrigTime(double time) {m_trigTime = time;}
        void setTrigFreq(double freq) {m_trigFreq = freq;}
        void setTrigDelay(uint32_t delay, uint32_t cal_edge_delay);
        void setEdgeMode(uint32_t duration);
        void setNoInject();

        void writeConfig(json &config) override;
        void loadConfig(const json &config) override;

        uint32_t getExpEvents() override;

    private:
        uint32_t m_trigDelay;
        uint32_t m_calEdgeDelay;
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
        bool m_zeroTot;

        void init() override;
        void end() override;
        void execPart1() override;
        void execPart2() override;
};

#endif
