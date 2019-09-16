#ifndef STARMASKLOOP_H
#define STARMASKLOOP_H

#include <iostream>

#include "StarChips.h"
#include "LoopActionBase.h"

enum MASK_STAGE {
    MASK_1  = 0xFFFFFFFF,
    MASK_2  = 0x55555555,
    MASK_4  = 0x11111111,
    MASK_8  = 0x01010101,
    MASK_16 = 0x00010001,
    MASK_32 = 0x00000001,
    MASK_NONE    = 0x00000000
};

enum CAL_EN_STAGE {
	CAL_EN_1  = 0x00000000,
	CAL_EN_2  = 0xCCCCCCCC,
	CAL_EN_4  = 0x11111111,
	CAL_EN_8  = 0x01010101,
	CAL_EN_16 = 0x00010001,
	CAL_EN_32 = 0x00000001,
	CAL_EN_NONE  = 0x00000000
};

class StarMaskLoop : public LoopActionBase {
    public:
        StarMaskLoop();

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
