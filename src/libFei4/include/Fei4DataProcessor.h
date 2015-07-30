#ifndef FEI4DATAPROCESSOR_H
#define FEI4DATAPROCESSOR_H

// #################################
// # Author: Timon Heim
// # Email: timon.heim at cern.ch
// # Project: Yarr
// # Description: FEI4 Data Processor
// # Comment: Takes
// ################################

#include <vector>
#include <array>
#include <map>

#include "DataProcessor.h"
#include "ClipBoard.h"
#include "RawData.h"
#include "Fei4EventData.h"

class Fei4DataProcessor : public DataProcessor {
    public:
        Fei4DataProcessor(unsigned arg_hitDiscCfg);
        ~Fei4DataProcessor();
        
        void connect(ClipBoard<RawDataContainer> *arg_input, std::map<unsigned, ClipBoard<Fei4Data> > *arg_outMap) {
            input = arg_input;
            outMap = arg_outMap;
        }
    
        void init();
        void process();

    private:
        ClipBoard<RawDataContainer> *input;
        std::map<unsigned, ClipBoard<Fei4Data> > *outMap;
        std::vector<unsigned> activeChannels;
        unsigned hitDiscCfg;
        std::array<std::array<unsigned, 16>, 3> totCode;
        std::map<unsigned, unsigned> l1id;
        std::map<unsigned, unsigned> bcid;
        std::map<unsigned, unsigned> wordCount;
        std::map<unsigned, int> hits;        
};

#endif
