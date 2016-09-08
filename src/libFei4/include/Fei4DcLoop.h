/*
 * Authors: T. Heim <timon.heim@cern.ch>,
 * Date: 2013-Oct-22
 */

#ifndef FEI4DCLOOP_H
#define FEI4DCLOOP_H

#include "LoopActionBase.h"
#include "Fei4.h"

class Fei4DcLoop: public LoopActionBase {
    public:
        Fei4DcLoop();
       
        void setMode(enum DC_MODE mode);
        uint32_t getMode();
        
        void loadConfig(json &config);
        void writeConfig(json &config);

    private:
        uint32_t m_mode;
        unsigned m_col;

        void init();
        void end();
        void execPart1();
        void execPart2();
};

#endif

