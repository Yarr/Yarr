#ifndef EMPTY_FRONTEND_DATA_PROCESSOR_H
#define EMPTY_FRONTEND_DATA_PROCESSOR_H

#include "FeDataProcessor.h"

class EmptyDataProcessor : public FeDataProcessor {
    public:
        void connect(FrontEndCfg *feCfg, ClipBoard<RawDataContainer> *arg_input, ClipBoard<EventDataBase> *arg_output) override {}

        /** Start thread */
        void run() override {};
        /** Complete thread (stop signalled via ClipBoard) */
        void join() override {};
};

#endif
