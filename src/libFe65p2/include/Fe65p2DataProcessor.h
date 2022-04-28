#ifndef FE65P2DATAPROCESSOR_H
#define FE65P2DATAPROCESSOR_H

// #################################
// # Author: Timon Heim
// # Email: timon.heim at cern.ch
// # Project: Yarr
// # Description: FE65P2 Data Processor
// # Comment: 
// ################################

#include <thread>

#include "DataProcessor.h"
#include "ClipBoard.h"
#include "RawData.h"
#include "EventDataBase.h"

class Fe65p2DataProcessor : public DataProcessor {
    public:
        Fe65p2DataProcessor();
        ~Fe65p2DataProcessor() override;

        void connect(ClipBoard<RawDataContainer> *arg_input, ClipBoard<EventDataBase> *arg_output) override {
            input = arg_input;
            output = arg_output;
        }

        void init() override;
        void run() override;
        void join() override;
        void process() override;
        void process_core();

    private:
        std::unique_ptr<std::thread> thread_ptr;
        ClipBoard<RawDataContainer> *input;
        ClipBoard<EventDataBase>  *output;
        unsigned tag;
        unsigned l1id;
        unsigned bcid;
        unsigned wordCount;
        unsigned hits;        

};

#endif
