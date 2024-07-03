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

#include "FeDataProcessor.h"
#include "ClipBoard.h"
#include "RawData.h"
#include "HccCfg.h"

/**
   Process Star front-end data to generate hits.

   Receive blocks of front end data for one link. Parse packets and
   generate hit data.

   LP and PR packets are decoded as expected. Mapping from input channels
   to channel number is according to the configuration.

   Counter registers are decoded to generate the appropriate number of
   hits to reconstruct the occupancy (FIXME).

   Other registers, and HPRs are ignored (FIXME).
 */
class StarDataProcessor : public FeDataProcessor {
    public:
        StarDataProcessor();
        ~StarDataProcessor() override;

        /// Connect this instance to data for a particular FrontEnd
        void connect(FrontEndCfg *feCfg, ClipBoard<RawDataContainer> *arg_input, ClipBoard<EventDataBase> *arg_output) override;
        void connect(ClipBoard<FeedbackProcessingInfo> *arg_proc_status) override {statusFb = arg_proc_status;}
    
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
