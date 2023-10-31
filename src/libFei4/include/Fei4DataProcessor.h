#ifndef FEI4DATAPROCESSOR_H
#define FEI4DATAPROCESSOR_H

// #################################
// # Author: Timon Heim
// # Email: timon.heim at cern.ch
// # Project: Yarr
// # Description: FEI4 Data Processor
// # Comment: Takes
// ################################

#include <array>
#include <map>
#include <thread>
#include <vector>

#include "DataProcessor.h"
#include "ClipBoard.h"
#include "RawData.h"

class Fei4DataProcessor : public DataProcessor {
    public:
        // TODO processor should receive whole chip config seperatly
        Fei4DataProcessor(unsigned arg_hitDiscCfg=0);
        ~Fei4DataProcessor() override;
        
        void connect(FrontEndCfg *feCfg, ClipBoard<RawDataContainer> *arg_input, ClipBoard<EventDataBase> *arg_output) override {
            input = arg_input;
            output = arg_output;
        }
        void connect(ClipBoard<FeedbackProcessingInfo> *arg_proc_status) override {statusFb = arg_proc_status;}
    
        void init() override;
        void run() override;
        void join() override;
        void process() override;
        void process_core();

    private:
        std::unique_ptr<std::thread> thread_ptr;
        ClipBoard<RawDataContainer> *input;
        ClipBoard<EventDataBase> *output;
        ClipBoard<FeedbackProcessingInfo> *statusFb = nullptr;
        unsigned hitDiscCfg;
        std::array<std::array<unsigned, 16>, 3> totCode;
        unsigned tag;
        unsigned l1id;
        unsigned bcid;
        unsigned wordCount;
        unsigned hits;        
        inline void sendFeedback(unsigned tag, unsigned bcid);
};

#endif
