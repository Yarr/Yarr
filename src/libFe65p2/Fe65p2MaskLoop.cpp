/*
 * Authors: T. Heim <timon.heim@cern.ch>
 * Date: 2016-Mar-22
 */

#include "Fe65p2MaskLoop.h"

Fe65p2MaskLoop::Fe65p2MaskLoop() : LoopActionBase() {
    m_mask = 0x0101;
    min = 0;
    max = 8;
    step = 1;
    m_cur =0;
    m_done = false;

    loopType = typeid(this);
}

void Fe65p2MaskLoop::init() {
    // TODO want to init mask here and then only shift it in execPart2()
    // Nothing to do
    m_cur = min;
    m_done = false;
}

void Fe65p2MaskLoop::end() {
    // Nothing to do
}

void Fe65p2MaskLoop::execPart1() {
    std::cout << " ---> Mask Stage " << m_cur << std::endl;
    // TODO needs to be done per FE!
    // Set threshold high
    uint16_t tmp1 = keeper->globalFe<Fe65p2>()->getValue(&Fe65p2::Vthin1Dac);
    std::cout << tmp1 << std::endl;
    uint16_t tmp2 = keeper->globalFe<Fe65p2>()->getValue(&Fe65p2::Vthin2Dac);
    uint16_t tmp3 = keeper->globalFe<Fe65p2>()->getValue(&Fe65p2::VffDac);
    keeper->globalFe<Fe65p2>()->setValue(&Fe65p2::Vthin1Dac, 255);
    keeper->globalFe<Fe65p2>()->setValue(&Fe65p2::Vthin2Dac, 0);
    keeper->globalFe<Fe65p2>()->setValue(&Fe65p2::VffDac, 0);
    // All in parallel
    keeper->globalFe<Fe65p2>()->setValue(&Fe65p2::ColEn, 0xFFFF);
    keeper->globalFe<Fe65p2>()->setValue(&Fe65p2::ColSrEn, 0xFFFF);
    // Write to global regs
    keeper->globalFe<Fe65p2>()->configureGlobal();
    usleep(2000); // Wait for DAC 
    // Write mask to SR
    /*
    uint16_t mask[16];
    for (unsigned i=0; i<16; i++)
        mask[i] = 0;
    mask[m_cur/16] = (0x1 << (m_cur%16));
    keeper->globalFe<Fe65p2>()->writePixel(mask);
    */
    keeper->globalFe<Fe65p2>()->writePixel((m_mask<<m_cur));
            
    // Write to Pixel reg
    keeper->globalFe<Fe65p2>()->setValue(&Fe65p2::InjEnLd, 0x1);
    keeper->globalFe<Fe65p2>()->setValue(&Fe65p2::PixConfLd, 0x3);
    keeper->globalFe<Fe65p2>()->configureGlobal();
    
    // Unset shadow reg and reset threshold
    keeper->globalFe<Fe65p2>()->setValue(&Fe65p2::PixConfLd, 0x0);
    keeper->globalFe<Fe65p2>()->setValue(&Fe65p2::InjEnLd, 0x0);
    keeper->globalFe<Fe65p2>()->setValue(&Fe65p2::Vthin1Dac, tmp1);
    keeper->globalFe<Fe65p2>()->setValue(&Fe65p2::Vthin2Dac, tmp2);
    keeper->globalFe<Fe65p2>()->setValue(&Fe65p2::VffDac, tmp3);
    keeper->globalFe<Fe65p2>()->configureGlobal();
    usleep(5000); // Wait for DAC 

    // Leave SR set, as it enables the digital inj (if TestHit is set)
    while(g_tx->isCmdEmpty() == 0);
    g_stat->set(this, m_cur);
    
}

void Fe65p2MaskLoop::execPart2() {
   m_cur += step;
   if (!((int)m_cur<max)) m_done = true;
}
