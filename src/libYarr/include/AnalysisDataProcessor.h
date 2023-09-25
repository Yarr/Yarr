#ifndef DATA_PROCESSOR_ANALYSIS_H
#define DATA_PROCESSOR_ANALYSIS_H

#include "DataProcessor.h"

#include "ClipBoard.h"
#include "HistogramBase.h"
#include "ScanBase.h"

class AnalysisDataProcessor : public DataProcessor {
    public:
        virtual void connect(ScanBase *arg_s, ClipBoard<HistogramBase> *arg_input, ClipBoard<HistogramBase> *arg_output) = 0;
};

#endif
