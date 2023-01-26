#ifndef RD53APARAMETERLOOP_H
#define RD53APARAMETERLOOP_H
// #################################
// # Author: Timon Heim
// # Email: timon.heim at cern.ch
// # Project: Yarr
// # Description: Parameter Loop for RD53A
// # Date: 03/2018
// ################################

#include <iostream>

#include "LoopActionBase.h"
#include "Rd53a.h"

class Rd53aParameterLoop : public LoopActionBase {
    public:
        Rd53aParameterLoop();
        Rd53aParameterLoop(Rd53Reg Rd53aGlobalCfg::*ref);

        void writeConfig(json &j) override;
        void loadConfig(const json &j) override;

    private:
        Rd53Reg Rd53aGlobalCfg::*parPtr{};
        std::string parName;
        void writePar();
        unsigned m_cur{};

        void init() override;
        void end() override;
        void execPart1() override;
        void execPart2() override;
};

#endif
