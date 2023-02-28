#ifndef RD53BPARAMETERLOOP_H
#define RD53BPARAMETERLOOP_H

// #################################
// # Author: Timon Heim
// # Email: timon.heim at cern.ch
// # Project: Yarr
// # Description: Parameter Loop for RD53B
// # Date: 07/2020
// ################################

#include "LoopActionBase.h"
#include "Rd53b.h"

class Rd53bParameterLoop : public LoopActionBase {
    public:
        Rd53bParameterLoop();
        Rd53bParameterLoop(Rd53bRegDefault Rd53bGlobalCfg::*ref);

        void writeConfig(json &j) override;
        void loadConfig(const json &j) override;

    private:
        Rd53bRegDefault Rd53bGlobalCfg::*parPtr;
        std::string parName;
        void writePar();
        unsigned m_cur;

        void init() override;
        void end() override;
        void execPart1() override;
        void execPart2() override;
};

#endif
