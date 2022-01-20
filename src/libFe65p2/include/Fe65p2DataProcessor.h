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

        void connect(ClipBoard<RawDataContainer> *arg_input, std::map<unsigned, ClipBoard<EventDataBase> > *arg_outMap) override {
            input = arg_input;
            outMap = arg_outMap;
        }

        void init() override;
        void run() override;
        void join() override;
        void process() override;
        void process_core();

    private:
        std::vector<std::unique_ptr<std::thread>> thread_ptrs;
        ClipBoard<RawDataContainer> *input;
        std::map<unsigned, ClipBoard<EventDataBase> > *outMap;
        std::vector<unsigned> activeChannels;
        std::map<unsigned, unsigned> tag;
        std::map<unsigned, unsigned> l1id;
        std::map<unsigned, unsigned> bcid;
        std::map<unsigned, unsigned> wordCount;
        std::map<unsigned, int> hits;        

};

#endif
