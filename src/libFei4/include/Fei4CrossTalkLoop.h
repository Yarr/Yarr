#ifndef FEI4CROSSTALKLOOP_H
#define FEI4CROSSTALKLOOP_H

#include "LoopActionBase.h"

class Fei4CrossTalkLoop : public LoopActionBase {
public:
    Fei4CrossTalkLoop() {loopType = typeid(this);}

private:
    enum Direction {
        PIXEL_DOWN = 0, PIXEL_UP = 1, PIXEL_RIGHT = 2, PIXEL_LEFT = 3
    };

    unsigned int m_cur;
    uint32_t mask_cur;
    uint8_t dc_cur;

    void init();
    void execPart1();
    void execPart2();
    void end();

    void downOnePixel();
    void upOnePixel();
    void rightOnePixel();
    void leftOnePixel();
};

#endif
