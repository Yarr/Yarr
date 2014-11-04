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
#include <typeinfo>

#include "ScanBase.h"
#include "ClipBoard.h"
#include "DataProcessor.h"
#include "HistogramBase.h"
#include "Histo2d.h"
#include "Fei4Histogrammer.h"

#include "AllFei4Actions.h"
#include "AllStdActions.h"

class AnalysisAlgorithm {
    public:
        AnalysisAlgorithm() {};
        ~AnalysisAlgorithm() {};

        void connect(ClipBoard<HistogramBase> *out) {
            output = out;
        }
        virtual void init(ScanBase *s) {}
        virtual void processHistogram(HistogramBase *h) {}
        virtual void end() {}

    protected:
        ScanBase *scan;
        ClipBoard<HistogramBase> *output;
};

class Fei4Analysis : DataProcessor {
    public:
        Fei4Analysis();
        ~Fei4Analysis();
        
        void connect(ScanBase *arg_s, ClipBoard<HistogramBase> *arg_input, ClipBoard<HistogramBase> *arg_output) {
            scan = arg_s;
            input = arg_input;
            output = arg_output;
        }
        
        void init();
        void process();

        void addAlgorithm(AnalysisAlgorithm *a);
        void plot(std::string basename);

    private:
        ClipBoard<HistogramBase> *input;
        ClipBoard<HistogramBase> *output;
        ScanBase *scan;
        
        std::vector<AnalysisAlgorithm*> algorithms;
};

class OccupancyAnalysis : public AnalysisAlgorithm {
    public:
        OccupancyAnalysis() : AnalysisAlgorithm() {};
        ~OccupancyAnalysis() {};

        void init(ScanBase *s);
        void processHistogram(HistogramBase *h);
        void end() {}
    private:
        std::vector<unsigned> loops;
        std::vector<unsigned> loopMax;
        unsigned n_count;
        std::map<unsigned, Histo2d*> occMaps;
        std::map<unsigned, unsigned> innerCnt;
};

class ScurveFitter : public AnalysisAlgorithm {
    public:
        ScurveFitter() : AnalysisAlgorithm() {};
        ~ScurveFitter() {};

        void init(ScanBase *s);
        void processHistogram(HistogramBase *h);
        void end();

    private:
        unsigned vcalLoop;
        unsigned vcalMin;
        unsigned vcalMax;
        unsigned vcalBins;
        unsigned n_count;
        std::vector<unsigned> loops;
        std::vector<unsigned> loopMax;
        std::map<unsigned, Histo1d*> histos;
        std::map<unsigned, unsigned> innerCnt;
};

#endif
