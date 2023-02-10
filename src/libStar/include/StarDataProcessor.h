#ifndef ABC_STAR_DATA_PROCESSOR_H
#define ABC_STAR_DATA_PROCESSOR_H

// #################################
// # Author:
// # Email:
// # Project: Yarr
// # Description: Strip Data Processor
// # Comment:
// ################################

#include <vector>
#include <array>
#include <map>
#include <thread>

#include "DataProcessor.h"
#include "ClipBoard.h"
#include "RawData.h"
#include "HccCfg.h"

class StarDataProcessor : public DataProcessor {
    public:
        StarDataProcessor();
        ~StarDataProcessor() override;

        /// Connect this instance to data for a particular FrontEnd
        void connect(FrontEndCfg *feCfg, ClipBoard<RawDataContainer> *arg_input, ClipBoard<EventDataBase> *arg_output) override;
        void connect(unsigned id, std::shared_ptr<ClipboardMapProcessingFeedback> arg_proc_statuses) override {statusFb = &(*arg_proc_statuses)[id];}
    
        void init() override;
        void run() override;
        void join() override;
        void process() override;
        virtual void process_core();

    private:
        ClipBoard<RawDataContainer> *input;
        ClipBoard<EventDataBase> *output;
        ClipBoard<FeedbackProcessingInfo> *statusFb = nullptr;

        std::unique_ptr<std::thread> thread_ptr;

        /// Map from HCC input channel (0-10) number to histogram slot
        std::array<uint8_t, HCC_INPUT_CHANNEL_COUNT> chip_map;
};

#endif
