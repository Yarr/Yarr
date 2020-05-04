#ifndef YARR_ANALYSIS_ALGORITHM_H
#define YARR_ANALYSIS_ALGORITHM_H

#include <thread>

#include "Bookkeeper.h"
#include "DataProcessor.h"
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

/**
 * Receive a sequence of histograms and process them using AnalysisAlgorithms.
 */
class AnalysisProcessor : public DataProcessor {
    public:
        AnalysisProcessor();
        AnalysisProcessor(Bookkeeper *b, unsigned ch);
        ~AnalysisProcessor();

        void connect(ScanBase *arg_s, ClipBoard<HistogramBase> *arg_input, ClipBoard<HistogramBase> *arg_output) {
            scan = arg_s;
            input = arg_input;
            output = arg_output;
        }

        void init();
        void run();
	void loadConfig(json &j);
        void join();
        void process();
        void process_core();
        void end();

        void addAlgorithm(std::unique_ptr<AnalysisAlgorithm> a);

        void setMapSize(unsigned col, unsigned row) {
            for (unsigned i=0; i<algorithms.size(); i++) {
                algorithms[i]->setMapSize(col, row);
            }
        }

        void setMasking(bool val) {
            for (unsigned i=0; i<algorithms.size(); i++) {
                algorithms[i]->setMasking(val);
            }
        }

    private:
        Bookkeeper *bookie;
        unsigned channel;
        ClipBoard<HistogramBase> *input;
        ClipBoard<HistogramBase> *output;
        ScanBase *scan;
        std::unique_ptr<std::thread> thread_ptr;
        
        std::vector<std::unique_ptr<AnalysisAlgorithm>> algorithms;
};

#endif
