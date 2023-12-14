#ifndef DATA_PROCESSOR_HISTO_H
#define DATA_PROCESSOR_HISTO_H

#include "DataProcessor.h"

#include "ClipBoard.h"
#include "EventDataBase.h"
#include "HistogramBase.h"

class HistoDataProcessor : public DataProcessor {
    public:
        virtual void connect(ClipBoard<EventDataBase> *arg_input, ClipBoard<HistogramBase> *arg_output) = 0;
};

#endif
