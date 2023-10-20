#ifndef DATAPROCESSOR_H
#define DATAPROCESSOR_H

// #################################
// # Author: Timon Heim
// # Email: timon.heim at cern.ch
// # Project: Yarr
// # Description: DataProc base class
// # Comment: Operates on data from the clipboard
// ################################

#include "FrontEnd.h"

class DataProcessor {
    public:
        DataProcessor();
        virtual ~DataProcessor() = default;;

        virtual void connect(ClipBoard<FeedbackProcessingInfo> *arg_proc_status) {}
        virtual void init() {}
        virtual void process() {}
        virtual void run() = 0;
        virtual void join() = 0;
};

#endif
