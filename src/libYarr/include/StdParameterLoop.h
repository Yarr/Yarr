#ifndef STD_PARAMETER_LOOP_H
#define STD_PARAMETER_LOOP_H

// #################################
// # Author: Bruce Gallop
// # Project: Yarr
// # Description: Generic Named Parameter Loop
// ################################

#include "LoopActionBase.h"
#include "FrontEnd.h"

class StdParameterLoop : public LoopActionBase {
    public:
        StdParameterLoop();

        void writeConfig(json &j) override;
        void loadConfig(json &j) override;
        std::string getParName() {return parName;}

    private:
        std::string parName;
        void writePar();

        void init() override;
        void end() override;
        void execPart1() override;
        void execPart2() override;

        unsigned m_cur;
        std::chrono::microseconds m_waitTime;
};

#endif
