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
        uint32_t getMode() const;
        
        void loadConfig(const json &config) override;
        void writeConfig(json &config) override;

    private:
        uint32_t m_mode;
        unsigned m_col;

        void init() override;
        void end() override;
        void execPart1() override;
        void execPart2() override;
};

#endif

