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

        void feedback(unsigned channel, Histo2d *h) {
            if (h->size() != 26880) {
                std::cout << __PRETTY_FUNCTION__ << " --> ERROR : Wrong type of feedback histogram!" << std::endl;
                doneMap[channel] = true;
            } else {
                fbHistoMap[channel] = h;
            }

            keeper->mutexMap[channel].unlock();
        }

    private:
        void init() {
            m_done = false;
            unsigned int ch;

            // Loop over active FEs
            for(unsigned int k=0; k<keeper->feList.size(); k++) {
                if(keeper->feList[k]->getActive()) {
                    ch = keeper->feList[k]->getChannel();
                    
                    // Init Maps
                    oldStepMap[ch] = stepMap[ch];
                    doneMap[ch] = false;
                    fbHistoMap[ch] = NULL;
                    stepMap[ch] = step;
                    
                    // Initilize Pixel regs with default config
                    for (unsigned col=1; col<81; col++) {
                        for (unsigned row=1; row<337; row++) {
                            this->setPixel(keeper->feList[k], col, row, min);
                        }
                    }
                }
            }


        }

        void end() {
        }

        void execPart1() {
            // TODO Current Step or iteration count ?
            g_stat->set(this, step);
         
            for(unsigned int k=0; k<keeper->feList.size(); k++) {
                if(keeper->feList[k]->getActive()) {
                    // Need to lock mutex on first itereation
                    keeper->mutexMap[keeper->feList[k]->getChannel()].try_lock();
                    // Write config
                    this->writePixelCfg(keeper->feList[k]);
                }
            }
        }

        void execPart2() {
            unsigned ch;
            for(unsigned int k=0; k<keeper->feList.size(); k++) {
                if(keeper->feList[k]->getActive()) {
                    ch = keeper->feList[k]->getChannel();

                    // Wait for Mutex to be unlocked by feedback
                    keeper->mutexMap[ch].lock();

                    this->addFeedback(ch);
                    std::cout << " Received feeddback for channel #" << ch << " at step: " << step << std::endl;

                    m_done = this->allDone();
                    if (lastMap[ch]) doneMap[ch] = true;
                    if (stepMap[ch] == 1 && oldStepMap[ch] == 1)
                        lastMap[ch] = true;
                    oldStepMap[ch] = stepMap[ch];
                    stepMap[ch] = stepMap[ch]/2;
                    if (stepMap[ch] ==0) stepMap[ch] =1;
                }
            }
        }

        bool allDone() {
            for(unsigned int k=0; k<keeper->feList.size(); k++) {
                if(keeper->feList[k]->getActive()) {
                    unsigned ch = keeper->feList[k]->getRxChannel();
                    if (!doneMap[ch])
                        return false;
                }
            }
            return true;
        }

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
            std::cout << " Feedback! at channel #" << ch << std::endl;
            if (fbHistoMap[ch] != NULL) {
                for (unsigned row=1; row<337; row++) {
                    for (unsigned col=1; col<81; col++) {
                        int sign = fbHistoMap[ch]->getBin(fbHistoMap[ch]->binNum(col, row));
                        int v = getPixel(keeper->getFe(ch),col, row);
                        v = v + ((int)stepMap[ch])*sign;
                        if (v < 0) v = 0;
                        if ((unsigned)v > max) v = max;
                        this->setPixel(keeper->getFe(ch),col, row, v);
                    }
                }
            }
            delete fbHistoMap[ch];
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
            g_tx->setCmdEnable(1 << fe->getTxChannel());
            fe->configurePixels(lsb, msb+1);
            g_tx->setCmdEnable(keeper->getTxMask());
        }

        enum FeedbackType fbType;
        Histo2d *fbHisto;
        unsigned oldStep;
        bool last;

        std::map<unsigned, Histo2d*>	fbHistoMap;
        std::map<unsigned, unsigned> oldStepMap;
        std::map<unsigned, unsigned> stepMap;
        std::map<unsigned, double> oldSign;
        std::map<unsigned, bool>	lastMap;

};

#endif
