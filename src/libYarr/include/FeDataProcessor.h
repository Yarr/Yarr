#ifndef DATA_PROCESSOR_FE_H
#define DATA_PROCESSOR_FE_H

#include "DataProcessor.h"

#include "ClipBoard.h"
#include "FrontEnd.h"
#include "RawData.h"

class FeDataProcessor : public DataProcessor {
    public:
        virtual void connect(FrontEndCfg *feCfg, ClipBoard<RawDataContainer> *arg_input, ClipBoard<EventDataBase> *arg_output) = 0;
};

#endif
