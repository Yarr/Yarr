#ifndef DATAPROCESSOR_H
#define DATAPROCESSOR_H

// #################################
// # Author: Timon Heim
// # Email: timon.heim at cern.ch
// # Project: Yarr
// # Description: DataProc base class
// # Comment: Operates on data from the clipboard
// ################################

#include <mutex>
/* #include <thread> */
#include <condition_variable>

#include "ClipBoard.h"
#include "RawData.h"
#include "EventDataBase.h"
#include "FeedbackBase.h"
#include "HistogramBase.h"
#include "ScanBase.h"

#define PROCESSING_FEEDBACK_TRIGGER_TAG_ERROR  -10
#define PROCESSING_FEEDBACK_TRIGGER_TAG_RR      -2
#define PROCESSING_FEEDBACK_TRIGGER_TAG_Control -3

class DataProcessor {
    public:
        DataProcessor();
        virtual ~DataProcessor() = default;;

        // TODO there must be a nicer way for this
        virtual void connect(FrontEndCfg *feCfg, ClipBoard<RawDataContainer> *arg_input, ClipBoard<EventDataBase> *arg_output) {}
        virtual void connect(ClipBoard<EventDataBase> *arg_input, ClipBoard<HistogramBase> *arg_output) {}
        virtual void connect(ScanBase *arg_s, ClipBoard<HistogramBase> *arg_input, ClipBoard<HistogramBase> *arg_output) {}
        virtual void connect(unsigned id, std::shared_ptr<ClipboardMapProcessingFeedback> arg_proc_statuses) {}
        virtual void init() {}
        virtual void process() {}
        virtual void run() = 0;
        virtual void join() = 0;

    protected:
        std::condition_variable cv;
        std::mutex mtx;

};

#endif
