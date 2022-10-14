/*
 * Authors: A. Toldaiev <alex.toldaiev@cern.ch>,
 * Date: 2022-Sep-5
 */

#ifndef STDDATALOOPFEEDBACK_H
#define STDDATALOOPFEEDBACK_H

#include "StdDataLoop.h"
#include "FeedbackBase.h"

class StdDataLoopFeedback: public StdDataLoop, public DataProcFeedbackReceiver {
    private:
        void execPart2() override;
        void dataproc_feedback(unsigned channel, struct DataProcFeedbackParams p) override;
};

#endif


