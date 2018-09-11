/*
 * Authors: T. Heim <timon.heim@cern.ch>,
 * Date: 2014-Nov-24
 */

#ifndef FEEDBACKBASE_H
#define FEEDBACKBASE_H

#include "Histo2d.h"
#include "LoopActionBase.h"

class GlobalFeedbackBase {
    public:
        virtual void feedback(unsigned channel, double sign, bool last) = 0;
        virtual void feedbackBinary(unsigned channel, double sign, bool last) = 0; // TODO Algorithm should be selected in scan
        virtual void feedbackStep(unsigned channel, double sign, bool last) {}
};

class PixelFeedbackBase {
    public:
        virtual void feedback(unsigned channel, Histo2d *h) {};
        virtual void feedbackStep(unsigned channel, Histo2d *h) {};
};

#endif

