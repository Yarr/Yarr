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
#include <cmath>
#include <functional>
#include <chrono>

#include "ScanBase.h"
#include "ClipBoard.h"
#include "DataProcessor.h"
#include "HistogramBase.h"
#include "Histo2d.h"
#include "Fei4Histogrammer.h"
#include "lmcurve.h"

#include "Bookkeeper.h"
#include "FeedbackBase.h"

#include "AllFei4Actions.h"
#include "AllFe65p2Actions.h"
#include "AllStdActions.h"

class AnalysisAlgorithm {
    public:
        AnalysisAlgorithm() {
            nCol = 80;
            nRow = 336;
            make_mask = true;
        };
        virtual ~AnalysisAlgorithm() {}
        
        void setBookkeeper (Bookkeeper *b) {bookie = b;}
        void setChannel (unsigned ch) {channel = ch;}

        void connect(ClipBoard<HistogramBase> *out) {
            output = out;
        }
        virtual void init(ScanBase *s) {}
        virtual void processHistogram(HistogramBase *h) {}
        virtual void end() {}

        void setMapSize(unsigned col,unsigned row) {
            nCol = col;
            nRow = row;
        }

        void enMasking() {make_mask = true;}
        void disMasking() {make_mask = false;}

    protected:
        Bookkeeper *bookie;
        unsigned channel;
        ScanBase *scan;
        ClipBoard<HistogramBase> *output;
        bool make_mask;
        unsigned nCol, nRow;
};

class Fei4Analysis : DataProcessor {
    public:
        Fei4Analysis();
        Fei4Analysis(Bookkeeper *b, unsigned ch);
        ~Fei4Analysis();
        
        void connect(ScanBase *arg_s, ClipBoard<HistogramBase> *arg_input, ClipBoard<HistogramBase> *arg_output) {
            scan = arg_s;
            input = arg_input;
            output = arg_output;
        }
        
        void init();
        void process();
        void end();

        void addAlgorithm(AnalysisAlgorithm *a);
		void addAlgorithm(AnalysisAlgorithm *a, unsigned ch);
        void plot(std::string basename, std::string dir = "");
        void toFile(std::string basename, std::string dir = "");

        void setMapSize(unsigned col, unsigned row) {
            for (unsigned i=0; i<algorithms.size(); i++) {
                algorithms[i]->setMapSize(col, row);
            }
        }

        AnalysisAlgorithm* getLastAna() {return algorithms.back();}
            

    private:
        Bookkeeper *bookie;
        unsigned channel;
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
        unsigned injections;
        std::map<unsigned, Histo2d*> occMaps;
        std::map<unsigned, unsigned> innerCnt;
};

class TotAnalysis : public AnalysisAlgorithm {
    public:
        TotAnalysis() : AnalysisAlgorithm() {};
        ~TotAnalysis() {};

        void init(ScanBase *s);
        void processHistogram(HistogramBase *h);
        void end() {}
    private:
        std::vector<unsigned> loops;
        std::vector<unsigned> loopMax;
        unsigned n_count;
        double injections;
        std::map<unsigned, Histo2d*> occMaps;
        std::map<unsigned, unsigned> occInnerCnt;
        std::map<unsigned, Histo2d*> totMaps;
        std::map<unsigned, unsigned> totInnerCnt;
        std::map<unsigned, Histo2d*> tot2Maps;
        std::map<unsigned, unsigned> tot2InnerCnt;
        GlobalFeedbackBase *globalFb;
        PixelFeedbackBase *pixelFb;
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
        unsigned vcalStep;
        unsigned vcalBins;
        unsigned n_count;
        unsigned injections;
        unsigned cnt;
        std::vector<double> x;
        std::vector<unsigned> loops;
        std::vector<unsigned> loopMax;
        std::map<unsigned, Histo1d*> histos;
        std::map<unsigned, Histo2d*> thrMap;
        std::map<unsigned, Histo1d*> thrDist;
        std::map<unsigned, Histo2d*> sigMap;
        std::map<unsigned, Histo1d*> sigDist;
        std::map<unsigned, Histo1d*> chiDist;
        std::map<unsigned, Histo1d*> timeDist;
        std::map<unsigned, unsigned> innerCnt;
};

class OccGlobalThresholdTune : public AnalysisAlgorithm {
    public:
        OccGlobalThresholdTune() : AnalysisAlgorithm()  {};
        ~OccGlobalThresholdTune() {};

        void init(ScanBase *s);
        void processHistogram(HistogramBase *h);
        void end() {};

    private:
        std::vector<unsigned> loops;
        std::vector<unsigned> loopMax;
        unsigned n_count;
        std::map<unsigned, Histo2d*> occMaps;
        std::map<unsigned, Histo1d*> occDists;
        std::map<unsigned, unsigned> innerCnt;
        unsigned injections;
        GlobalFeedbackBase *fb;
        LoopActionBase *lb;

};

class GlobalPreampTune : public AnalysisAlgorithm {
    public:
        GlobalPreampTune() : AnalysisAlgorithm()  {};
        ~GlobalPreampTune() {};

        void init(ScanBase *s);
        void processHistogram(HistogramBase *h);
        void end() {};

    private:
        std::vector<unsigned> loops;
        std::vector<unsigned> loopMax;
        unsigned n_count;
        std::map<unsigned, Histo2d*> occMaps;
        std::map<unsigned, Histo2d*> totMaps;
        std::map<unsigned, Histo1d*> occDists;
        std::map<unsigned, unsigned> innerCnt;
        unsigned injections;
        GlobalFeedbackBase *fb;

};

class OccPixelThresholdTune : public AnalysisAlgorithm {
    public:
        OccPixelThresholdTune() : AnalysisAlgorithm()  {};
        ~OccPixelThresholdTune() {};

        void init(ScanBase *s);
        void processHistogram(HistogramBase *h);
        void end() {};

    private:
        std::vector<unsigned> loops;
        std::vector<unsigned> loopMax;
        unsigned n_count;
        std::map<unsigned, Histo2d*> occMaps;
        std::map<unsigned, unsigned> innerCnt;
        unsigned injections;
        PixelFeedbackBase *fb;

};

class L1Analysis : public AnalysisAlgorithm {
    public:
        L1Analysis() : AnalysisAlgorithm() {};
        ~L1Analysis() {};

        void init(ScanBase *s);
        void processHistogram(HistogramBase *h);
        void end() {}
    private:
        std::vector<unsigned> loops;
        std::vector<unsigned> loopMax;
        unsigned n_count;
        unsigned injections;
        std::map<unsigned, Histo1d*> l1Histos;
        std::map<unsigned, unsigned> innerCnt;
};

class TotDistPlotter : public AnalysisAlgorithm {
    public:
        TotDistPlotter() : AnalysisAlgorithm() {};
        ~TotDistPlotter() {};

        void init(ScanBase *s);
        void processHistogram(HistogramBase *h);
        void end() {}
    private:
        std::vector<unsigned> loops;
        std::vector<unsigned> loopMax;
        unsigned n_count;
        unsigned injections;
        std::map<unsigned, Histo1d*> tot;
        std::map<unsigned, unsigned> innerCnt;
};

class NoiseAnalysis : public AnalysisAlgorithm {
    public:
        NoiseAnalysis() : AnalysisAlgorithm() {};
        ~NoiseAnalysis() {};

        void init(ScanBase *s);
        void processHistogram(HistogramBase *h);
        void end();
    private:
        unsigned n_trigger;
        Histo2d* occ;        
};

#endif
