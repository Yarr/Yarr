#include "Fei4CrossTalkLoop.h"

void Fei4CrossTalkLoop::init(){
    m_cur = Direction::PIXEL_DOWN;
    mask_cur = 0x00000001;
    dc_cur = 4; //4, 5, 6, 7 is equivalent to 0, 1, 2, 3 (because colpr_mode is 01)
                //but this version has no overflow problems

    return;
}

void Fei4CrossTalkLoop::execPart1(){
    switch(m_cur){
        case Direction::PIXEL_DOWN:
            this->downOnePixel();
            break;
        case Direction::PIXEL_UP:
            this->upOnePixel();
            break;
        case Direction::PIXEL_RIGHT:
            this->rightOnePixel();
            break;
        case Direction::PIXEL_LEFT:
            this->leftOnePixel();
            break;
        default:
            break;
    }

    return;
}

void Fei4CrossTalkLoop::execPart2(){

        switch(m_cur){
            case Direction::PIXEL_DOWN:
                this->upOnePixel();
                break;
            case Direction::PIXEL_UP:
                this->downOnePixel();
                break;
            case Direction::PIXEL_RIGHT:
                this->leftOnePixel();
                break;
            case Direction::PIXEL_LEFT:
                this->rightOnePixel();
                break;
            default:
                break;
        }

    if(m_cur == Direction::PIXEL_LEFT){
        m_done = true;
        m_cur = Direction::PIXEL_DOWN;
        if(dc_cur == 7){
            dc_cur = 4;
            mask_cur = mask_cur << 1;
        }else{
            dc_cur += 1;
        }
    }else{
        m_cur += 1;
    }

    return;
}

void Fei4CrossTalkLoop::end(){

    return;
}

void Fei4CrossTalkLoop::downOnePixel(){
    if(mask_cur == 0x00000001){
        mask_cur = 0x80000000;
    }else{
        mask_cur = mask_cur >> 1;
    }

    g_fe->initMask(mask_cur);
    g_fe->loadIntoPixel(1);

    return;
}

void Fei4CrossTalkLoop::upOnePixel(){
    if(mask_cur == 0x80000000){
        mask_cur = 0x00000001;
    }else{
        mask_cur = mask_cur << 1;
    }

    g_fe->initMask(mask_cur);
    g_fe->loadIntoPixel(1);

    return;
}

void Fei4CrossTalkLoop::rightOnePixel(){
    dc_cur += 1;
    g_fe->writeRegister(&Fei4::Colpr_Addr, dc_cur);

    return;
}

void Fei4CrossTalkLoop::leftOnePixel(){
//    if(dc_cur == 0){
//        dc_cur = 3;
//    }else{
        dc_cur -= 1;
//    }
    g_fe->writeRegister(&Fei4::Colpr_Addr, dc_cur);

    return;
}
