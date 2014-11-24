/*
 * Authors: T. Heim <timon.heim@cern.ch>,
 * Date: 2014-Nov-24
 */

#ifndef FEI4PIXELFEEDBACK_H
#define FEI4PIXELFEEDBACK_H

#include <queue>
#include <mutex>
#include "LoopActionBase.h"
#include "Histo2d.h"

enum FeedbackType {
    TDAC_FB, // 0 - 31
    FDAC_FB// 0 - 15
};

class Fei4PixelFeedback : public LoopActionBase {
    public:
        Fei4PixelFeedback(enum FeedbackType type) {
            fbType = type;
            switch (fbType) {
                case (TDAC_FB):
                    step = 16;
                    min = 15;
                    break;
                case (FDAC_FB):
                    step = 8;
                    min = 7;
                    break;
            }
            max = 0;
        }

        void feedback(Histo2d h) {
            if (h.size() != 26880) {
                std::cout << __PRETTY_FUNCTION__ << " --> ERROR : Wrong type of feedback histogram!" << std::endl;
                m_done = true;
            } else {
                for (unsigned col=1; col<=80; col++) {
                    for (unsigned row=1; row<=336; row++) {
                        int sign = h.getBin(row+(col*336));
                        this->setPixel(col, row, (this->getPixel(col, row)+(step*sign)));
                    }
                }
            }
            fbMutex.unlock();
        }
        
    private:
        void init() {
            m_done = false;
            // Initilize Pixel regs with default config
            for (unsigned col=1; col<=80; col++) {
                for (unsigned row=1; row<=336; row++) {
                    this->setPixel(col, row, min);
                }
            }
            this->writePixelCfg();
        }

        void end() {//Save last config
        }

        void execPart1() {
            g_stat->set(this, step);
            fbMutex.try_lock(); // Need to lock on frist run
        }

        void execPart2() {
            fbMutex.lock();
            if (step == 1)
                m_done = true;
            step = step/2;
            this->writePixelCfg();
        }

        // TODO Make Multi channel capable
        unsigned getPixel(unsigned col, unsigned row) {
            unsigned v = 0;
            switch (fbType) {
                case (TDAC_FB):
                    v = g_fe->getTDAC(col, row);
                    break;
                case (FDAC_FB):
                    v = g_fe->getFDAC(col, row);
                    break;
            }
            return v;
        }

        void setPixel(unsigned col, unsigned row, unsigned v) {
            switch (fbType) {
                case (TDAC_FB):
                    g_fe->setTDAC(col, row, v);
                    break;
                case (FDAC_FB):
                    g_fe->setFDAC(col, row, v);
                    break;
            }
        }
        
        void writePixelCfg() {
            // Not real lsb/msb because that is defined in the pixel config
            unsigned lsb = 0;
            unsigned msb = 0;
            switch (fbType) {
                case (TDAC_FB):
                    lsb  = 1;
                    msb = 5;
                    break;
                case (FDAC_FB):
                    lsb = 9;
                    msb = 12;
                    break;
            }
            // Write config into FE
            g_fe->configurePixels(lsb, msb+1);
        }


        std::mutex fbMutex;
        enum FeedbackType fbType;
};

#endif
