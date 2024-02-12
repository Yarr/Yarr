#ifndef RD53A2TRIGGERLOOP_H
#define RD53A2TRIGGERLOOP_H

// #################################
// # Author: Timon Heim, Magne Lauritzen and Simon Huiberts
// # Email: timon.heim at cern.ch, magne.eik.laurizen at cern.ch, simon.kristian.huiberts at cern.ch
// # Project: Yarr
// # Description: Two Trigger Loop for RD53A
// # Date: 05/2020
// ################################

#include <array>
#include <chrono>
#include <thread>
#include <cmath>
#include "LoopActionBase.h"
#include "StdTriggerAction.h"
#include "Rd53a.h"
#include "Rd53aCmd.h"


class Rd53a2TriggerLoop: public LoopActionBase, public StdTriggerAction {
    public:
        Rd53a2TriggerLoop();

        void setTrigTime(double time) {m_trigTime = time;}
        void setTrigFreq(double freq) {m_trigFreq = freq;}
        void setTrigDelay(uint32_t delay);
        uint32_t getTrigCnt() const override {return m_trigCnt*2;}
        void verifyParameters();

        void setAutozeroPulse();
        void setNoInject();
        void doubleCmdInject();
        void singleCmdInject();
        void flexibleTrigger(uint8_t offset, int injDelay, int triggers, int delay);
        uint8_t findSmallestDelay() const;
        uint8_t greatestDelay() const;

        void writeConfig(json &config) override;
        void loadConfig(const json &config) override;

    private:
        //User parameters
        uint32_t m_trigDelay;
        uint32_t m_trigDelay2;

        float m_trigTime;
        float m_trigFreq;
        bool m_noInject;
        bool m_noInject2;
        bool m_extTrig;
        bool m_synPulse;
        float m_doubleDelay;
        uint32_t m_Ntrig1; //Number of triggers after injection 1
        uint32_t m_Ntrig2; //Number of triggers after injection 2

        uint8_t CMDDEL = 5; //Constant injection delay for edge mode 1. Time in bunch crossings it takes from cal command to be received to it being executed.
        std::array<uint32_t, 32> m_trigWord{};
        std::array<uint8_t, 32> m_trigPulses{};
        uint32_t m_trigWordLength;
        uint32_t m_edgeDuration{};
        uint32_t m_edgeDelay{};
        uint32_t m_auxDelay{};
        uint32_t m_debugParamAdder{};
        bool m_sendEcr; 

        bool isInner;
        void init() override;
        void execPart1() override;
        void execPart2() override;
        void end() override;
};

#endif
