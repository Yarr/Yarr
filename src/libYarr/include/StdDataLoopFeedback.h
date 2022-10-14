/*
 * Authors: A. Toldaiev <alex.toldaiev@cern.ch>,
 * Date: 2022-Sep-5
 */

#ifndef STDDATALOOPFEEDBACK_H
#define STDDATALOOPFEEDBACK_H

#include "StdDataLoop.h"
#include "FeedbackBase.h"

class StdDataLoopFeedback: public StdDataLoop, public ReceiverOfRawDataProcessingFeedback {
    private:
        void execPart2() override;
};

#endif


