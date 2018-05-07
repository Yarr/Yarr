/*
 * Authors: T. Heim <timon.heim@cern.ch>,
 * Date: 2014-May-17
 */

#include "Fe65p2PixelFeedback.h"

Fe65p2PixelFeedback::Fe65p2PixelFeedback() {
    step = 8;
    min = 16;
    max = 31;

    cur = 0;
    m_done = false;

    loopType = typeid(this);
}

void Fe65p2PixelFeedback::feedback(unsigned channel, Histo2d *h) {
    std::cout << "--> Received feedback on channel: " << channel << std::endl;
    // TODO Check on NULL pointer
    if (h->size() != 4096) {
        std::cout << __PRETTY_FUNCTION__ 
            << " --> ERROR : Wrong type of feedback histogram on channel " << channel << std::endl;
        doneMap[channel] = true;
    } else {
        fbHistoMap[channel] = h;
    }

    keeper->mutexMap[channel].unlock();
}

void Fe65p2PixelFeedback::setPixel(unsigned channel, unsigned col, unsigned row, unsigned val) {
   // Fe65p2 TDAC encoding
   unsigned dsign = 0;
   unsigned tdac = 15;
   if (val >15) {
       dsign = 0;
       tdac = val - 16;
   } else {
       dsign = 1;
       tdac = 15-val;
   }
   
   dynamic_cast<Fe65p2*>(keeper->feList[channel])->setSign(col, row, dsign);
   dynamic_cast<Fe65p2*>(keeper->feList[channel])->setTDAC(col, row, tdac);
   keeper->globalFe<Fe65p2>()->setSign(col, row, dsign);
   keeper->globalFe<Fe65p2>()->setTDAC(col, row, tdac);
}

unsigned Fe65p2PixelFeedback::getPixel(unsigned channel, unsigned col, unsigned row) {
   // Fe65p2 TDAC encoding
   unsigned dsign = keeper->globalFe<Fe65p2>()->getSign(col, row);
   unsigned tdac = keeper->globalFe<Fe65p2>()->getTDAC(col, row);
    
   unsigned val = 32;
   if (dsign == 0) {
        val = tdac + 16;
   } else {
       val = 15 - tdac;
   }
   return val;
}

void Fe65p2PixelFeedback::writePixelCfg(unsigned channel) {
    // Turn off all pixels
    keeper->globalFe<Fe65p2>()->setValue(&Fe65p2::ColEn, 0xFFFF);
    keeper->globalFe<Fe65p2>()->setValue(&Fe65p2::ColSrEn, 0xFFFF);
    keeper->globalFe<Fe65p2>()->writePixel((uint16_t)0x0);
    keeper->globalFe<Fe65p2>()->setValue(&Fe65p2::PixConfLd, 0x3);
    keeper->globalFe<Fe65p2>()->configureGlobal();
    keeper->globalFe<Fe65p2>()->setValue(&Fe65p2::PixConfLd, 0x0);
    keeper->globalFe<Fe65p2>()->configureGlobal();
    
    // Reconfigure pixels
    keeper->globalFe<Fe65p2>()->configurePixels();
}

void Fe65p2PixelFeedback::addFeedback(unsigned ch) {
    if (fbHistoMap[ch] != NULL) {
        for (unsigned row=1; row<65; row++) {
            for (unsigned col=1; col<65; col++) {
                int sign = fbHistoMap[ch]->getBin(fbHistoMap[ch]->binNum(col, row));
                int v = this->getPixel(ch,col, row);
                v = v + (step)*(-1*sign); // Invert as we are using FE-I4 analysis
                if (v < 0) v = 0;
                if (v > max) v = max;
                this->setPixel(ch,col, row, v);
                //std::cout << v << " ";
            }
            //std::cout << std::endl;
        }
        delete fbHistoMap[ch];
        fbHistoMap[ch] = NULL;
    }
}

void Fe65p2PixelFeedback::init() {
    m_done = false;
    cur = 0;

    unsigned ch = 0; // hardcoded TODO
    fbHistoMap[ch] = NULL;
    for (unsigned col = 1; col<65; col++) {
        for (unsigned row = 1; row<65; row++) {
            this->setPixel(ch, col, row, min);
        }
    }
    
}

void Fe65p2PixelFeedback::end() {

}

void Fe65p2PixelFeedback::execPart1() {
    g_stat->set(this, cur);
    unsigned ch = 0; // hardcoded TODO
    keeper->mutexMap[ch].try_lock();
    this->writePixelCfg(ch);
}

void Fe65p2PixelFeedback::execPart2() {
    unsigned ch = 0;
    keeper->mutexMap[ch].lock();
    this->addFeedback(ch);

    // Execute last step twice to get full range
    if (step == 1 && oldStep == 1)
        m_done = true;
    oldStep = step;
    step = step/2;
    if(step == 0)
        step = 1;
    cur++;
}

