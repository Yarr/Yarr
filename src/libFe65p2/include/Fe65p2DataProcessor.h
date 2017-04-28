#ifndef FE65P2DATAPROCESSOR_H
#define FE65P2DATAPROCESSOR_H

// #################################
// # Author: Timon Heim
// # Email: timon.heim at cern.ch
// # Project: Yarr
// # Description: FE65P2 Data Processor
// # Comment: 
// ################################

#include "DataProcessor.h"
#include "ClipBoard.h"
#include "RawData.h"
#include "Fei4EventData.h"

class Fe65p2DataProcessor : public DataProcessor {
    public:
        Fe65p2DataProcessor();
        ~Fe65p2DataProcessor();

        void connect(ClipBoard<RawDataContainer> *arg_input, std::map<unsigned, ClipBoard<Fei4Data> > *arg_outMap) {
            input = arg_input;
            outMap = arg_outMap;
        }

        void init();
        void run();
        void join();
        void process();
        void process_core();

        static bool scanDone;
           
    private:
        std::vector<std::unique_ptr<std::thread>> thread_ptrs;
        ClipBoard<RawDataContainer> *input;
        std::map<unsigned, ClipBoard<Fei4Data> > *outMap;
        std::vector<unsigned> activeChannels;
        std::map<unsigned, unsigned> tag;
        std::map<unsigned, unsigned> l1id;
        std::map<unsigned, unsigned> bcid;
        std::map<unsigned, unsigned> wordCount;
        std::map<unsigned, int> hits;        

};

#endif
