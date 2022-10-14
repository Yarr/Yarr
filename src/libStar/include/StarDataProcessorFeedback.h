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

class StarDataProcessorFeedback : public StarDataProcessor {
    public:
        void connect(ClipBoard<FeedbackProcessingInfo> *arg_proc_status) override {statusFb = arg_proc_status;}

        //void process_core();
        void process_data(RawData &curIn,
                  FrontEndData &curOut,
                  const std::array<uint8_t, 11> &chip_map);

    private:
        ClipBoard<FeedbackProcessingInfo> *statusFb = nullptr;
};

#endif
