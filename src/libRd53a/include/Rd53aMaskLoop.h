#ifndef RD53AMASKLOOP_H
#define RD53AMASKLOOP_H
// #################################
// # Author: Timon Heim
// # Email: timon.heim at cern.ch
// # Project: Yarr
// # Description: Mask Loop for RD53A
// # Date: 03/2018
// ################################

#include <iostream>
#include <vector>
#include <tuple>
#include <algorithm>
#include <iterator>

#include "FrontEnd.h"
#include "Rd53a.h"
#include "LoopActionBase.h"

class Rd53aMaskLoop : public LoopActionBase {
    public:
        Rd53aMaskLoop();

        void writeConfig(json &j);
        void loadConfig(json &j);
    private:
        unsigned m_cur;
        std::vector<int> goodPixels;

        void init();
        void end();
        void execPart1();
        void execPart2();
        void calcNruns();
        
        std::map<FrontEnd*, std::array<uint16_t, Rd53a::n_DC*Rd53a::n_Row>> m_pixRegs;
};

#endif
