#ifndef DATAPROCESSOR_H
#define DATAPROCESSOR_H

// #################################
// # Author: Timon Heim
// # Email: timon.heim at cern.ch
// # Project: Yarr
// # Description: DataProc base class
// # Comment: Operates on data from the clipboard
// ################################

#include "storage.hpp"

/**
 * Class to encapsulate processing of data.
 *
 * This is normally implemented by taking data from an input queue
 * applying a function and putting the result into an output queue.
 */
class DataProcessor {
    public:
        DataProcessor();
        virtual ~DataProcessor() = default;

        /** Do initialisation */
        virtual void init() {}
        /** Process all the data in the input clipboard */
        virtual void process() {}
        /** Start thread */
        virtual void run() = 0;
        /** Complete thread (stop signalled via ClipBoard) */
        virtual void join() = 0;
        /** Retrieve log for global scan log post processing */
        virtual json getLog() {json log; return log;}
};

#endif
