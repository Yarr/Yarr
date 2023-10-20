#ifndef DATA_PROCESSOR_FE_H
#define DATA_PROCESSOR_FE_H

#include "DataProcessor.h"

#include "ClipBoard.h"
#include "FrontEnd.h"
#include "RawData.h"

/**
 * Data processor of front end data.
 *
 * An implementation will receive raw data from the hardware via the
 * `RawDataContainer` clipboard. Feedback to the data collection process
 * can be provided via the `FeedbackProcessingInfo` clipboard.
 *
 * The output is sent as a series of `EventDataBase` objects.
 */
class FeDataProcessor : public DataProcessor {
    public:
        virtual void connect(FrontEndCfg *feCfg, ClipBoard<RawDataContainer> *arg_input, ClipBoard<EventDataBase> *arg_output) = 0;
        virtual void connect(ClipBoard<FeedbackProcessingInfo> *arg_proc_status) {}
};

#endif
