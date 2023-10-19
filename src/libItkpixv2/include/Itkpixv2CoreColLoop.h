#ifndef ITKPIXV2CORECOLLOOP_H
#define ITKPIXV2CORECOLLOOP_H

// #################################
// # Author: Timon Heim
// # Email: timon.heim at cern.ch
// # Project: Yarr
// # Description: Loop over core columns for ITkPixV2
// # Date: 07/2023
// ################################

#include "LoopActionBase.h"

class Itkpixv2CoreColLoop : public LoopActionBase {
    public:
        Itkpixv2CoreColLoop();

        void writeConfig(json &j) override;
        void loadConfig(const json &j) override;

    private:
        unsigned m_cur;
        unsigned m_nSteps;
        unsigned m_minCore;
        unsigned m_maxCore;
        bool m_usePToT;
        bool m_disUnused;

        std::array<uint16_t, 4> m_coreCols;

        void init() override;
        void end() override;
        void execPart1() override;
        void execPart2() override;

        void setCores();
};

#endif
