#ifndef ABC_STAR_DATA_PROCESSOR_FEEDBACK_H
#define ABC_STAR_DATA_PROCESSOR_FEEDBACK_H

// #################################
// # Author:
// # Email:
// # Project: Yarr
// # Description: Strip Data Processor
// # Comment:
// ################################

#include "StarDataProcessor.h"
#include "FeedbackBase.h"
#include "EventData.h"

class StarDataProcessorFeedback : public StarDataProcessor {
    public:
        void connect(ClipBoard<FeedbackProcessingInfo> *arg_proc_status) override {statusFb = arg_proc_status;}

        void process_core();
        void process_data(RawData &curIn, FrontEndData &curOut);

    private:
        ClipBoard<FeedbackProcessingInfo> *statusFb = nullptr;
};

#endif
