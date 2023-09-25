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

class DataProcessor {
    public:
        DataProcessor();
        virtual ~DataProcessor() = default;;

        virtual void init() {}
        virtual void process() {}
        virtual void run() = 0;
        virtual void join() = 0;

    protected:
        std::condition_variable cv;
        std::mutex mtx;

};

#endif
