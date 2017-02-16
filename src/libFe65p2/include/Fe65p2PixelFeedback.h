/*
 * Authors: T. Heim <timon.heim@cern.ch>,
 * Date: 2014-May-17
 */

#ifndef FE65P2PIXELFEEDBACK_H
#define FE65P2PIXELFEEDBACK_H

#include "LoopActionBase.h"
#include "FeedbackBase.h"
#include "Histo2d.h"
#include "Fe65p2.h"

class Fe65p2PixelFeedback : public LoopActionBase, public PixelFeedbackBase {
    public:
        Fe65p2PixelFeedback();

        void feedback(unsigned channel, Histo2d *h);
    private:
        std::map<unsigned, Histo2d*> fbHistoMap;
        unsigned step, oldStep;
        unsigned cur;

        void setPixel(unsigned channel, unsigned col, unsigned row, unsigned val);
        unsigned getPixel(unsigned channel, unsigned col, unsigned row);
        void writePixelCfg(unsigned channel);
        void addFeedback(unsigned channel);

        void init();
        void end();
        void execPart1();
        void execPart2();
};

#endif
