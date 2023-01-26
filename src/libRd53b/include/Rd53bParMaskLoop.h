#ifndef RD53BPARMASKLOOP_H
#define RD53BPARMASKLOOP_H

// #################################
// # Author: Timon Heim
// # Email: timon.heim at cern.ch
// # Project: Yarr
// # Description: Parallel Mask Loop for RD53B
// # Date: 07/2022
// ################################

#include <vector>
#include <array>
#include <map>

#include "LoopActionBase.h"
#include "Rd53b.h"
#include "FrontEnd.h"

class Rd53bParMaskLoop: public LoopActionBase {
    public:
        Rd53bParMaskLoop();

        void writeConfig(json &j) override;
        void loadConfig(const json &j) override;

    private:
        unsigned m_cur;
        std::map<FrontEnd*, std::array<std::array<uint16_t, Rd53b::n_Row>, Rd53b::n_DC> > m_pixRegs;
        int m_maskType;
        bool m_applyEnMask;
        
        void init() override;
        void end() override;
        void execPart1() override;
        void execPart2() override;

        bool applyMask(unsigned col, unsigned row);

};
#endif
