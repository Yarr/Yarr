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
        unsigned hitDiscCfg;
        std::array<std::array<unsigned, 16>, 3> totCode;
        std::map<unsigned, unsigned> tag;
        std::map<unsigned, unsigned> l1id;
        std::map<unsigned, unsigned> bcid;
        std::map<unsigned, unsigned> wordCount;
        std::map<unsigned, int> hits;        
};

#endif
