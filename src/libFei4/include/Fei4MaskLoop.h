/*
 * Authors: T. Heim <timon.heim@cern.ch>,
 * Date: 2014-Sep-27
 */

#ifndef FEI4MASKLOOP_H
#define FEI4MASKLOOP_H

#include <iostream>

#include "Fei4.h"
#include "LoopActionBase.h"

class Fei4MaskLoop : public LoopActionBase {
    public:
        Fei4MaskLoop();

        void setMaskStage(enum MASK_STAGE mask);
        void setMaskStage(uint32_t mask);
        //uint32_t getMaskStage();
        //void setIterations(unsigned it);
        //unsigned getIterations();
        void setScap(bool v=true) {enable_sCap = v;}
        bool getScap() {return enable_sCap;}
        void setLcap(bool v=true) {enable_lCap = v;}
        bool getLcap() {return enable_lCap;}

        void writeConfig(json &config);
        void loadConfig(json &config);
        
    private:
        uint32_t m_mask;
        unsigned m_cur;
        bool enable_sCap;
        bool enable_lCap;

        void init();
        void end();
        void execPart1();
        void execPart2();
};

#endif
