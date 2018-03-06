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
#include "LoopActionBase.h"
#include "Rd53aCmd.h"

class Rd53aTriggerLoop: public LoopActionBase {
    public:
        Rd53aTriggerLoop();
        
        void setTrigCnt(uint32_t cnt) {m_trigCnt = cnt;}
        void setTrigTime(double time) {m_trigTime = time;}
        void setTrigFreq(double freq) {m_trigFreq = freq;}
        void setTrigDelay(uint32_t delay);
        
        void writeConfig(json &config);
        void loadConfig(json &config);

    private:
        uint32_t m_trigCnt;
        uint32_t m_trigDelay;
        double m_trigTime;
        double m_trigFreq;
        std::array<uint32_t, 16> m_trigWord;
        uint32_t m_trigWordLength;

        bool isInner;
        void init();
        void execPart1();
        void execPart2();
        void end();
};

#endif
