#ifndef RD53A2TRIGGERLOOP_H
#define RD53A2TRIGGERLOOP_H

// #################################
// # Author: Timon Heim and Magne Lauritzen
// # Email: timon.heim at cern.ch, magne.eik.laurizen at cern.ch
// # Project: Yarr
// # Description: Two Trigger Loop for RD53A
// # Date: 12/2018
// ################################

#include <array>
#include <chrono>
#include <thread>
#include <math.h>
#include "LoopActionBase.h"
#include "Rd53a.h"
#include "Rd53aCmd.h"


class Rd53a2TriggerLoop: public LoopActionBase {
    public:
        Rd53a2TriggerLoop();

        uint32_t getTrigCnt() {return m_trigCnt;}
        void setTrigCnt(uint32_t cnt) {m_trigCnt = cnt;}
        void setTrigTime(double time) {m_trigTime = time;}
        void setTrigFreq(double freq) {m_trigFreq = freq;}
        void setTrigDelay(uint32_t delay, bool isSecond);

        void setNoInject();
        void doubleCmdInject();
        void singleCmdInject();
        void setFlexibleTrigger(uint8_t offset, int injDelay);
        uint8_t findSmallestDelay();
    
        void writeConfig(json &config);
        void loadConfig(json &config);

    private:
        //User parameters
        uint32_t m_trigCnt;
        uint32_t m_trigDelay;
        int m_realTrigDelay;
        double m_trigTime;
        double m_trigFreq;
        bool m_noInject;
        bool m_extTrig;
        float m_doubleDelay;
        uint32_t m_Ntrig;

        uint8_t CMDDEL = 5; //Constant injection delay for edge mode 1. Time in bunch crossings it takes from cal command to be received to it being executed.
        std::array<uint32_t, 32> m_trigWord;
        std::array<uint8_t, 32> m_trigPulses;
        uint32_t m_trigWordLength;
        uint32_t m_doubleInjectOffset;
        uint32_t m_edgeDuration;
        uint32_t m_edgeDelay;
        uint32_t m_auxDelay;
        uint32_t m_debugParamAdder;
        
        bool isInner;
        void init();
        void execPart1();
        void execPart2();
        void end();
};

#endif
