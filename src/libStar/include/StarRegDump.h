#ifndef STARREGDUMP_H
#define STARREGDUMP_H

// #################################
// # Author:
// # Email:
// # Project: Yarr
// # Description: TriggerLoop ABCStar
// # Comment:
// ################################

#include "LoopActionBase.h"
#include "StarChips.h"

class StarRegDump: public LoopActionBase {
    public:
        StarRegDump();
        void writeConfig(json &config) override;
        void loadConfig(const json &config) override;

    private:
        int    m_addr;
        void init() override;
        void end() override;
        void execPart1() override;
        void execPart2() override;
};

#endif

