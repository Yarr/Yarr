#ifndef YARR_ANALYSIS_ALGORITHM_H
#define YARR_ANALYSIS_ALGORITHM_H

#include "Bookkeeper.h"
#include "HistogramBase.h"
#include "ScanBase.h"

/**
 * Process sequence of histograms.
 * 
 * Output is a set of histograms distinguished by name.
 */
class AnalysisAlgorithm {
    public:
        AnalysisAlgorithm() {
            nCol = 80;
            nRow = 336;
            make_mask = true;
        }
        virtual ~AnalysisAlgorithm() {}
        
        void setBookkeeper (Bookkeeper *b) {bookie = b;}
        void setChannel (unsigned ch) {channel = ch;}

        void connect(ClipBoard<HistogramBase> *out) {
            output = out;
        }
        virtual void init(ScanBase *s) {}
	virtual void loadConfig(json &config){}
        virtual void processHistogram(HistogramBase *h) {}
        virtual void end() {}

        void setMapSize(unsigned col,unsigned row) {
            nCol = col;
            nRow = row;
        }

        void enMasking() {make_mask = true;}
        void disMasking() {make_mask = false;}
        void setMasking(bool val) {make_mask = val;}

    protected:
        Bookkeeper *bookie;
        unsigned channel;
        ScanBase *scan;
        ClipBoard<HistogramBase> *output;
        bool make_mask;
        unsigned nCol, nRow;
};

#endif
