#ifndef DATA_PROCESSOR_FE_H
#define DATA_PROCESSOR_FE_H

#include "DataProcessor.h"

#include <functional>

#include "ClipBoard.h"
#include "EventDataBase.h"
#include "FrontEnd.h"
#include "RawData.h"

#include "storage.hpp"

class FrontEndCfg;

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
        /**
         * Connect FE configuration and input/output streams.
         *
         * @param feCfg Config for this instantiation of data processor.
         * @param arg_input Connect input stream (raw data)
         * @param arg_output Connect output stream (event data)
         */
        virtual void connect(FrontEndCfg *feCfg, ClipBoard<RawDataContainer> *arg_input, ClipBoard<EventDataBase> *arg_output) = 0;

        /**
         * Connect feedback stream.
         *
         * This can be used to provide feedback to the data loop, allowing
         * it to count reception of event data.
         *
         * @param arg_proc_status Connect processing info stream
         */
        virtual void connect(ClipBoard<FeedbackProcessingInfo> *arg_proc_status) {}

        /**
         * Optional core of event processor loop.
         *
         * Processes a sequence of raw data blocks and generates a sequence
         * of event data.
         *
         * Where appropriate, this can be used to process data without
         * sending data through a ClipBoard.
         *
         * @param rdc The container of sequence raw data.
         * @param push_fb Function to call to generate event data feedback.
         *
         * Note that this function is not currently implemented for all front
         * ends.
         */
        virtual std::unique_ptr<EventDataBase> process_event_core(const RawDataContainer &rdc, std::function<void (std::unique_ptr<FeedbackProcessingInfo>)> push_fb) { return {}; }

        /**
         * Configure the front-end DataProcessor.
         *
         * This is to be used to change parts of the decoding, for instance
         * enabling reading of raw/direct data, or changing handling for
         * register reads.
         */
        virtual void loadConfig(const json &config) {}
};

#endif
