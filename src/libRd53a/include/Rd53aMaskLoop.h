#ifndef RD53AMASKLOOP_H
#define RD53AMASKLOOP_H
// #################################
// # Author: Timon Heim
// # Email: timon.heim at cern.ch
// # Project: Yarr
// # Description: Mask Loop for RD53A
// # Date: 03/2018
// ################################

#include "LoopActionBase.h"

class Rd53aMaskLoop final : public LoopActionBase {
    public:
        Rd53aMaskLoop();

        void writeConfig(json & /*j*/) override final;
        void loadConfig (json & /*j*/) override final;
    private:
        unsigned m_cur;

        void init()      override final;
        void end()       override final;
        void execPart1() override final;
        void execPart2() override final;
};

#endif
