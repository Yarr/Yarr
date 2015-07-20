#ifndef DATAPROCESSOR_H
#define DATAPROCESSOR_H

// #################################
// # Author: Timon Heim
// # Email: timon.heim at cern.ch
// # Project: Yarr
// # Description: DataProc base class
// # Comment: Operates on data from the clipboard
// ################################

class DataProcessor {
    public:
        DataProcessor();
        virtual ~DataProcessor() {};

        virtual void init() {}
        virtual void process() {}
};

#endif
