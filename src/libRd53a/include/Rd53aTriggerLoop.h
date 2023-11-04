#ifndef RD53ATRIGGERLOOP_H
#define RD53ATRIGGERLOOP_H

// #################################
// # Author: Timon Heim
// # Email: timon.heim at cern.ch
// # Project: Yarr
// # Description: Trigger Loop for RD53A
// # Date: 02/2018
// ################################

#include <array>
#include <chrono>
#include <thread>
#include "LoopActionBase.h"
#include "StdTriggerAction.h"
#include "Rd53a.h"
#include "Rd53aCmd.h"

class Rd53aTriggerLoop: public LoopActionBase, public StdTriggerAction {
    public:
        Rd53aTriggerLoop();
 
        void setTrigTime(double time) {m_trigTime = time;}
        void setTrigFreq(double freq) {m_trigFreq = freq;}
        void setTrigDelay(uint32_t delay);
        void setEdgeMode(uint32_t duration);
        void setNoInject();
        
        void writeConfig(json &config) override;
        void loadConfig(const json &config) override;
        
        uint32_t getExpEvents() override;

    private:
        uint32_t m_trigDelay;
        float m_trigTime;
        float m_trigFreq;
        std::array<uint32_t, 32> m_trigWord{};
        uint32_t m_trigWordLength;
        bool m_noInject;
        bool m_edgeMode;
        bool m_extTrig;
        uint32_t m_edgeDuration;
        uint32_t m_pulseDuration;
        uint32_t m_trigMultiplier;
        bool m_sendEcr;

        bool isInner;
        void init() override;
        void execPart1() override;
        void execPart2() override;
        void end() override;
};

#endif
