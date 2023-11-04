#ifndef STD_PARAMETER_LOOP_H
#define STD_PARAMETER_LOOP_H

// #################################
// # Author: Bruce Gallop
// # Project: Yarr
// # Description: Generic Named Parameter Loop
// ################################

#include "LoopActionBase.h"
#include "StdParameterAction.h"

class StdParameterLoop : public LoopActionBase, public StdParameterAction {
    public:
        StdParameterLoop();

        void writeConfig(json &j) override;
        void loadConfig(const json &j) override;

    private:
        void writePar();

        void init() override;
        void end() override;
        void execPart1() override;
        void execPart2() override;

        unsigned m_cur;
        std::chrono::microseconds m_waitTime;
};

#endif
