#ifndef RD53BCORECOLLOOP_H
#define RD53BCORECOLLOOP_H

// #################################
// # Author: Timon Heim
// # Email: timon.heim at cern.ch
// # Project: Yarr
// # Description: Loop over core columns for RD53B
// # Date: 07/2020
// ################################

#include "LoopActionBase.h"

class Rd53bCoreColLoop : public LoopActionBase {
    public:
        Rd53bCoreColLoop();

        void writeConfig(json &j) override;
        void loadConfig(const json &j) override;

    private:
        unsigned m_cur = 0;
        unsigned m_nSteps = 0;
        unsigned m_minCore = 0;
        unsigned m_maxCore = 0;
        bool m_usePToT = false;
        bool m_disUnused = false;
        bool m_resetAtEnd = false;
        bool m_ignoreDis = false;
        bool m_skipDis = false;

        std::array<uint16_t, 4> m_coreCols = {};
        std::vector<std::array<uint16_t, 4>> m_initCoreColsAllChips;

        void init() override;
        void end() override;
        void execPart1() override;
        void execPart2() override;

        void setCores();
};

#endif
