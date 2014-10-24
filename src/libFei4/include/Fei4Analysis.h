#ifndef FEI4ANALYSIS_H
#define FEI4ANALYSIS_H

// #################################
// # Author: Timon Heim
// # Email: timon.heim at cern.ch
// # Project: Yarr
// # Description: Analysis Base class
// # Comment: 
// ################################

#include <iostream>
#include <vector>

#include "DataProcessor.h"
#include "HistogramBase.h"
#include "ResultBase.h"

class AnalysisAlgorithm {
    public:
        AnalysisAlgorithm() {};
        ~AnalysisAlgorithm() {};

        virtual void processHistogram(HistogramBase *h) {}

    protected:
        ResultBase *r;
};

class Fei4Analysis : DataProcessor {
    public:
        Fei4Analysis();
        ~Fei4Analysis();

        void init();
        void process();

        void addAlgorithm(AnalysisAlgorithm *a);

    private:
        std::vector<AnalysisAlgorithm*> algorithms;
};

#endif
