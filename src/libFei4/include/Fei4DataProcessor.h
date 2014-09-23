#ifndef FEI4DATAPROCESSOR_H
#define FEI4DATAPROCESSOR_H

// #################################
// # Author: Timon Heim
// # Email: timon.heim at cern.ch
// # Project: Yarr
// # Description: FEI4 Data Processor
// # Comment: Takes
// ################################

#include "DataProcessor.h"
#include "ClipBoard.h"
#include "RawData.h"
#include "Fei4EventData.h"

class Fei4DataProcessor : DataProcessor {
    public:
        Fei4DataProcessor();
        ~Fei4DataProcessor();
        
        void connect(ClipBoard<RawData> *arg_input, ClipBoard<Fei4Data> *arg_output) {
            input = arg_input;
            output = arg_output;
        }

        void process();

    private:
        ClipBoard<RawData> *input;
        ClipBoard<Fei4Data> *output;

};

#endif
