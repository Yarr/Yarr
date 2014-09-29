/*
 * Authors: T. Heim <timon.heim@cern.ch>,
 * Date: 2014-Sep-27
 */

#include "Fei4MaskLoop.h"

Fei4MaskLoop::Fei4MaskLoop() : LoopActionBase() {
    m_mask = MASK_16;
    m_it = 16;
    m_itCur = 0;
}

void Fei4MaskLoop::init() {
    if (verbose)
        std::cout << __PRETTY_FUNCTION__ << std::endl;
    // Shift Mask into all pixels
    g_fe->writeRegister(&Fei4::Colpr_Mode, 0x3);
    g_fe->writeRegister(&Fei4::Colpr_Addr, 0x0);
    g_fe->initMask(m_mask);
    g_fe->loadIntoPixel(0x1);
}

void Fei4MaskLoop::end() {
    if (verbose)
        std::cout << __PRETTY_FUNCTION__ << std::endl;
    // Disable all pixels
    g_fe->initMask(MASK_NONE);
    g_fe->loadIntoPixel(0x1);
}

void Fei4MaskLoop::execPart1() {
    if (verbose)
        std::cout << __PRETTY_FUNCTION__ << std::endl;
}

void Fei4MaskLoop::execPart2() {
    if (verbose)
        std::cout << __PRETTY_FUNCTION__ << std::endl;
    if (!(m_itCur < m_it)) m_done = true;
    m_itCur++;
    g_fe->shiftMask();
    g_fe->loadIntoPixel(0x1);
}

void Fei4MaskLoop::setMaskStage(enum MASK_STAGE mask) {
    switch (mask) {
        case MASK_1:
            m_mask = MASK_1;
            m_it = 1;
            break;
        case MASK_2:
            m_mask = MASK_2;
            m_it = 2;
            break;
        case MASK_4:
            m_mask = MASK_4;
            m_it = 4;
            break;
        case MASK_8:
            m_mask = MASK_8;
            m_it = 8;
            break;
        case MASK_16:
            m_mask = MASK_16;
            m_it = 16;
            break;
        case MASK_32:
            m_mask = MASK_32;
            m_it = 32;
            break;
        case MASK_NONE:
            m_mask = MASK_NONE;
            m_it = 0;
            break;
    }
}

void Fei4MaskLoop::setMaskStage(uint32_t mask) {
    m_mask = mask;
}

uint32_t Fei4MaskLoop::getMaskStage() {
    return m_mask;
}

void Fei4MaskLoop::setIterations(unsigned it) {
    m_it = it;
}

unsigned Fei4MaskLoop::getIterations() {
    return m_it;
}
