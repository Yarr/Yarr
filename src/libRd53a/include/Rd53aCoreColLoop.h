#ifndef RD53ACORECOLLOOP_H
#define RD53ACORECOLLOOP_H

// #################################
// # Author: Timon Heim
// # Email: timon.heim at cern.ch
// # Project: Yarr
// # Description: Loop over core columns for RD53A
// # Date: 03/2018
// ################################

#include <iostream>

#include "FrontEnd.h"
#include "Rd53a.h"
#include "LoopActionBase.h"

class Rd53aCoreColLoop : public LoopActionBase {
    public:
        Rd53aCoreColLoop();
        
        void writeConfig(json &j);
        void loadConfig(json &j);
    private:
        unsigned m_cur;
        unsigned nSteps;
        unsigned maxCore;
        unsigned minCore;
        void init();
        void end();
        void execPart1();
        void execPart2();
};


#endif

