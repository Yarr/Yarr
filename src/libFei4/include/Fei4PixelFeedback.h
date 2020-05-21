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
    static logging::Logger &logger() {
        static logging::LoggerStore instance = logging::make_log("Fei4PixelFeedback");
        return *instance;
    }

    public:
        Fei4PixelFeedback() : LoopActionBase(){
            loopType = typeid(this);
        }
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
                logger().error("Wrong type of feedback histogram on channel {}", channel);
                doneMap[channel] = true;
            } else {
                fbHistoMap[channel] = h;
            }

            fbMutexMap[channel].unlock();
        }
        void writeConfig(json &config){
	    config["min"]=min;
	    config["max"]=max;
            config["step"]=step;
            config["parameter"] = parName;
        }
        void loadConfig(json &config){
	    if (!config["min"].empty())
	      min = config["min"];
	    if (!config["max"].empty())
	      max = config["max"];
	    if (!config["step"].empty())
	      step = config["step"];
	    if (!config["parameter"].empty())
	      parName = config["parameter"];
	    if(parName=="TDAC_FB"){
	      fbType=TDAC_FB;
	    }
	    else if(parName=="FDAC_FB"){
	      fbType=FDAC_FB;
	    }
        }
    private:
	std::string parName="";
        void init() {
            m_done = false;
            cur = 0;

            // Loop over active FEs
            for(unsigned int k=0; k<keeper->feList.size(); k++) {
                auto fe = keeper->feList[k];
                if(fe->getActive()) {
                    unsigned ch = dynamic_cast<FrontEndCfg*>(fe)->getRxChannel();
                    
                    // Init Maps
                    fbHistoMap[ch] = NULL;
                    
                    // Initilize Pixel regs with default config
                    auto fei4 = dynamic_cast<Fei4*>(keeper->feList[k]);
                    for (unsigned col=1; col<81; col++) {
                        for (unsigned row=1; row<337; row++) {
                            this->setPixel(fei4, col, row, min);
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
                auto fe = keeper->feList[k];
                if(fe->getActive()) {
                    // Need to lock mutex on first itereation
                    fbMutexMap[dynamic_cast<FrontEndCfg*>(fe)->getRxChannel()].try_lock();
                    // Write config
                    this->writePixelCfg(dynamic_cast<Fei4*>(fe));
                }
            }
        }

        void execPart2() {
            unsigned ch;
            for(unsigned int k=0; k<keeper->feList.size(); k++) {
                auto fe = keeper->feList[k];
                if(fe->getActive()) {
                    ch = dynamic_cast<FrontEndCfg*>(fe)->getRxChannel();
                    // Wait for Mutex to be unlocked by feedback
                    fbMutexMap[ch].lock();
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
                auto fe = keeper->feList[k];
                if(fe->getActive()) {
                    unsigned ch = dynamic_cast<FrontEndCfg*>(fe)->getRxChannel();
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
            auto histo = fbHistoMap[ch];
            if (histo != NULL) {
                auto fe = dynamic_cast<Fei4*>(keeper->getFe(ch));
                for (unsigned row=1; row<337; row++) {
                    for (unsigned col=1; col<81; col++) {
                        int sign = histo->getBin(histo->binNum(col, row));
                        int v = getPixel(fe,col, row);
                        v = v + (step)*sign;
                        if (v < 0) v = 0;
                        if (v > max) v = max;
                        this->setPixel(fe, col, row, v);
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
            g_tx->setCmdEnable(dynamic_cast<FrontEndCfg*>(fe)->getTxChannel());
            fe->configurePixels(lsb, msb+1);
            g_tx->setCmdEnable(keeper->getTxMask());
            while(!g_tx->isCmdEmpty());
        }

        enum FeedbackType fbType;
        std::map<unsigned, Histo2d*> fbHistoMap;
        std::map<unsigned, std::mutex> fbMutexMap;
        unsigned step, oldStep;
        unsigned cur;

        // Somehow we need to register logger at static init time
        friend void logger_static_init_fei4();
};

#endif
