#ifndef DATA_PROCESSOR_ANALYSIS_H
#define DATA_PROCESSOR_ANALYSIS_H

#include "DataProcessor.h"

#include "ClipBoard.h"
#include "FeedbackBase.h"
#include "HistogramBase.h"
#include "ScanBase.h"

class AnalysisDataProcessor : public DataProcessor {
    public:
        virtual void connect(const ScanLoopInfo *arg_s, ClipBoard<HistogramBase> *arg_input, ClipBoard<HistogramBase> *arg_output, FeedbackClipboard *arg_fb, bool storeInput=false) = 0;
};

#endif
