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
        Rd53bParameterLoop(Rd53bReg Rd53bGlobalCfg::*ref);

        void writeConfig(json &j);
        void loadConfig(json &j);

    private:
        Rd53bReg Rd53bGlobalCfg::*parPtr;
        std::string parName;
        void writePar();
        unsigned m_cur;

        void init();
        void end();
        void execPart1();
        void execPart2();
};

#endif
