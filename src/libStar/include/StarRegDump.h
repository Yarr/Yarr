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
        void writeConfig(json &config);
        void loadConfig(json &config);

    private:
        int    m_addr;
        void init();
        void end();
        void execPart1();
        void execPart2();
};

#endif

