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
#include "FeedbackBase.h"

enum FeedbackType {
    TDAC_FB, // 0 - 31
    FDAC_FB// 0 - 15
};

class Fei4PixelFeedback : public LoopActionBase, public PixelFeedbackBase {
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
                    step = 4;
                    min = 8;
                    max = 15;
                    break;
            }
            loopType = typeid(this);
        }

        void feedback(unsigned channel, Histo2d *h) {
            // TODO Check on NULL pointer
            if (h->size() != 26880) {
                std::cout << __PRETTY_FUNCTION__ 
                    << " --> ERROR : Wrong type of feedback histogram on channel " << channel << std::endl;
                doneMap[channel] = true;
            } else {
                fbHistoMap[channel] = h;
            }

            keeper->mutexMap[channel].unlock();
        }

    private:
        void init() {
            m_done = false;
            cur = 0;

            // Loop over active FEs
            for(unsigned int k=0; k<keeper->feList.size(); k++) {
                if(keeper->feList[k]->getActive()) {
                    unsigned ch = dynamic_cast<FrontEndCfg*>(keeper->feList[k])->getRxChannel();
                    
                    // Init Maps
                    fbHistoMap[ch] = NULL;
                    
                    // Initilize Pixel regs with default config
                    for (unsigned col=1; col<81; col++) {
                        for (unsigned row=1; row<337; row++) {
                            this->setPixel(dynamic_cast<Fei4*>(keeper->feList[k]), col, row, min);
                        }
                    }
                }
            }


        }

        void end() {
        }

        void execPart1() {
            g_stat->set(this, cur);
         
            for(unsigned int k=0; k<keeper->feList.size(); k++) {
                if(keeper->feList[k]->getActive()) {
                    // Need to lock mutex on first itereation
                    keeper->mutexMap[dynamic_cast<FrontEndCfg*>(keeper->feList[k])->getRxChannel()].try_lock();
                    // Write config
                    this->writePixelCfg(dynamic_cast<Fei4*>(keeper->feList[k]));
                }
            }
        }

        void execPart2() {
            unsigned ch;
            for(unsigned int k=0; k<keeper->feList.size(); k++) {
                if(keeper->feList[k]->getActive()) {
                    ch = dynamic_cast<FrontEndCfg*>(keeper->feList[k])->getRxChannel();
                    // Wait for Mutex to be unlocked by feedback
                    keeper->mutexMap[ch].lock();
                    this->addFeedback(ch);
                }
            }
            // Execute last step twice to get full range
            if (step == 1 && oldStep == 1)
                m_done = true;
            oldStep = step;
            step = step/2;
            if(step == 0)
                step = 1;
            cur++;
        }

        bool allDone() {
            for(unsigned int k=0; k<keeper->feList.size(); k++) {
                if(keeper->feList[k]->getActive()) {
                    unsigned ch = dynamic_cast<FrontEndCfg*>(keeper->feList[k])->getRxChannel();
                    if (!doneMap[ch])
                        return false;
                }
            }
            return true;
        }

        unsigned getPixel(Fei4 *fe, unsigned col, unsigned row) {
            unsigned v = 0;
            switch (fbType) {
                case (TDAC_FB):
                    v = fe->getTDAC(col, row);
                    break;
                case (FDAC_FB):
                    v = fe->getFDAC(col, row);
                    break;
            }
            return v;
        }

        void setPixel(Fei4 *fe, unsigned col, unsigned row, unsigned v) {
            switch (fbType) {
                case (TDAC_FB):
                    fe->setTDAC(col, row, v);
                    break;
                case (FDAC_FB):
                    fe->setFDAC(col, row, v);
                    break;
            }
        }

        void addFeedback(unsigned ch) {
            if (fbHistoMap[ch] != NULL) {
                for (unsigned row=1; row<337; row++) {
                    for (unsigned col=1; col<81; col++) {
                        int sign = fbHistoMap[ch]->getBin(fbHistoMap[ch]->binNum(col, row));
                        int v = getPixel(dynamic_cast<Fei4*>(keeper->getFe(ch)),col, row);
                        v = v + (step)*sign;
                        if (v < 0) v = 0;
                        if (v > max) v = max;
                        this->setPixel(dynamic_cast<Fei4*>(keeper->getFe(ch)),col, row, v);
                    }
                }
                delete fbHistoMap[ch];
            }
        }

        void writePixelCfg(Fei4 *fe) {
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
            g_tx->setCmdEnable(1 << dynamic_cast<FrontEndCfg*>(fe)->getTxChannel());
            fe->configurePixels(lsb, msb+1);
            g_tx->setCmdEnable(keeper->getTxMask());
            while(!g_tx->isCmdEmpty());
        }

        enum FeedbackType fbType;
        std::map<unsigned, Histo2d*> fbHistoMap;
        unsigned step, oldStep;
        unsigned cur;
};

#endif
