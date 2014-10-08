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

#include "DataProcessor.h"
#include "ClipBoard.h"
#include "RawData.h"
#include "Fei4EventData.h"

class Fei4DataProcessor : public DataProcessor {
    public:
        Fei4DataProcessor(unsigned arg_hitDiscCfg);
        ~Fei4DataProcessor();
        
        void connect(ClipBoard<RawData> *arg_input, ClipBoard<Fei4Data> *arg_output) {
            input = arg_input;
            output = arg_output;
        }
    
        void init();
        void process();

    private:
        ClipBoard<RawData> *input;
        ClipBoard<Fei4Data> *output;
        unsigned hitDiscCfg;
        std::array<std::array<unsigned, 16>, 3> totCode;
};

#endif
