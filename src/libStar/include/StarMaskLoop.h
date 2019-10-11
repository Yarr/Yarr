#ifndef STARMASKLOOP_H
#define STARMASKLOOP_H

#include <iostream>

#include "StarChips.h"
#include "LoopActionBase.h"

enum MASK_STAGE_STRIP { //TODO-change this or delete it
    MASK_STRIP_1  = 0xFFFFFFFF
};


class StarMaskLoop : public LoopActionBase {
    public:
        StarMaskLoop();

        void setMaskStage(enum MASK_STAGE_STRIP mask);
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
