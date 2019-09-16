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
#include "StarChipPacket.h"

class StarDataProcessor : public DataProcessor {
    public:
        // TODO processor should receive whole chip config seperatly
        StarDataProcessor();
        ~StarDataProcessor();
        
        void connect(ClipBoard<RawDataContainer> *arg_input, std::map<unsigned, ClipBoard<EventDataBase> > *arg_outMap) {
            input = arg_input;
            outMap = arg_outMap;
        }
    
        void init() override;
        void run() override;
        void join() override;
        void process() override;
        void process_core();

    private:
        ClipBoard<RawDataContainer> *input;
        std::map<unsigned, ClipBoard<EventDataBase> > *outMap;
        std::vector<unsigned> activeChannels;
        std::vector<std::unique_ptr<std::thread>> thread_ptrs;
        std::map<unsigned, unsigned> tag;
        std::map<unsigned, unsigned> l1id;
        std::map<unsigned, unsigned> bcid;
        std::map<unsigned, unsigned> wordCount;
        std::map<unsigned, int> hits;
};

#endif
