#ifndef ITKPIXV2TRIGGERLOOP_H
#define ITKPIXV2TRIGGERLOOP_H

// #################################
// # Author: Timon Heim
// # Email: timon.heim at cern.ch
// # Project: Yarr
// # Description: Trigger Loop for ITKPIXV2
// # Date: 07/2023
// ################################

#include "LoopActionBase.h"
#include "StdTriggerAction.h"
#include "Itkpixv2.h"
#include "Itkpixv2Cmd.h"

class Itkpixv2TriggerLoop : public LoopActionBase, public StdTriggerAction {
    public:
        Itkpixv2TriggerLoop();
        
        void setTrigTime(double time) {m_trigTime = time;}
        void setTrigFreq(double freq) {m_trigFreq = freq;}
        void setTrigDelay(uint32_t delay, uint32_t cal_edge_delay);
        void setEdgeMode(uint32_t duration);
        void setNoInject();

        void writeConfig(json &config) override;
        void loadConfig(const json &config) override;

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
