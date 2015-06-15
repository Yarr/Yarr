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
#include "ClipBoard.h"

enum FeedbackType {
    TDAC_FB, // 0 - 31
    FDAC_FB// 0 - 15
};

class Fei4PixelFeedback : public LoopActionBase {
    public:
        Fei4PixelFeedback(enum FeedbackType type) : LoopActionBase(){
            fbType = type;
            switch (fbType) {
                case (TDAC_FB):
                    step = 8;
                    min = 16;
                    max = 31;
                    break;
                case (FDAC_FB):
                    step = 8;
                    min = 8;
                    max = 15;
                    break;
            }
            fbHisto = NULL;
            loopType = typeid(this);
        }

        void feedback(Histo2d *h) {
            if (h->size() != 26880) {
                std::cout << __PRETTY_FUNCTION__ << " --> ERROR : Wrong type of feedback histogram!" << std::endl;
                m_done = true;
            } else {
                fbHisto = h;
            }

            fbMutex.unlock();
        }
        
    private:
        void init() {
            m_done = false;
            // Initilize Pixel regs with default config
            for (unsigned col=1; col<81; col++) {
                for (unsigned row=1; row<337; row++) {
                    this->setPixel(col, row, min);
                }
            }
            this->writePixelCfg();
            oldStep = step;
        }

        void end() {//Save last config
        }

        void execPart1() {
            g_stat->set(this, step);
            fbMutex.try_lock(); // Need to lock on frist run
            this->writePixelCfg();
        }

        void execPart2() {
            fbMutex.lock();
            this->addFeedback();
            std::cout << " Received feeddback at step: " << step << std::endl;
            if (last) m_done = true;
            if (step == 1 && oldStep == 1)
                last = true;
            oldStep = step;
            step = step/2;
            if (step ==0) step =1;
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
                    g_fe->setTDAC(col, row, v);		// per FE
                    break;
                case (FDAC_FB):
                    g_fe->setFDAC(col, row, v);		// per FE
                    break;
            }
        }

        void addFeedback() {
            std::cout << " Feedback! " << std::endl;
            if (fbHisto != NULL) {
                for (unsigned row=1; row<337; row++) {
                    for (unsigned col=1; col<81; col++) {
                        int sign = fbHisto->getBin(fbHisto->binNum(col, row));
                        int v = getPixel(col, row);
                        v = v + ((int)step)*sign;
                        if (v < 0) v = 0;
                        if ((unsigned)v > max) v = max;
                        this->setPixel(col, row, v);
                    }
                }
            }
            delete fbHisto;
            std::cout << std::endl;
        }

        
        void writePixelCfg() {
            std::cout << "Writing config" << std::endl;
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
        Histo2d *fbHisto;
        unsigned oldStep;
        bool last;
};

#endif
