#ifndef RD53BMASKLOOP_H
#define RD53BMASKLOOP_H

// #################################
// # Author: Timon Heim
// # Email: timon.heim at cern.ch
// # Project: Yarr
// # Description: Mask Loop for RD53B
// # Date: 07/2020
// ################################

#include <vector>
#include <array>
#include <map>

#include "LoopActionBase.h"
#include "Rd53b.h"
#include "FrontEnd.h"

class Rd53bMaskLoop: public LoopActionBase {
    public:
        Rd53bMaskLoop();

        void writeConfig(json &j);
        void loadConfig(json &j);

    private:
        unsigned m_cur;
        std::map<FrontEnd*, std::array<std::array<uint16_t, Rd53b::n_Row>, Rd53b::n_DC> > m_pixRegs;

        void init();
        void end();
        void execPart1();
        void execPart2();

        bool applyMask(unsigned col, unsigned row);

};
#endif
