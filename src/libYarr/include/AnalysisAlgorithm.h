#ifndef YARR_ANALYSIS_ALGORITHM_H
#define YARR_ANALYSIS_ALGORITHM_H

#include <thread>

#include "Bookkeeper.h"
#include "AnalysisDataProcessor.h"
#include "FeedbackBase.h"
#include "HistogramBase.h"
#include "ScanBase.h"
#include "StdParameterLoop.h"

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
        virtual ~AnalysisAlgorithm() = default;
        
        void setBookkeeper (Bookkeeper *b) {bookie = b;}
        void setId (unsigned uid) {id = uid;}

        void connect(ClipBoard<HistogramBase> *out, FeedbackClipboard *fb) {
            output = out;
            feedback = fb;
        }
        virtual void init(ScanBase *s) {}
        virtual void loadConfig(const json &config){}
        virtual void processHistogram(HistogramBase *h) {}
        virtual void end() {}

        virtual bool requireDependency() {return false;}

        void setMapSize(unsigned col,unsigned row) {
            nCol = col;
            nRow = row;
        }

        void enMasking() {make_mask = true;}
        void disMasking() {make_mask = false;}
        void setMasking(bool val) {make_mask = val;}

    protected:
        Bookkeeper *bookie;
        unsigned id;
        ScanBase *scan;
        ClipBoard<HistogramBase> *output;
        FeedbackClipboard *feedback;
        bool make_mask;
        unsigned nCol, nRow;

        std::vector<std::string> m_parametersOfInterest;
        bool isPOILoop(StdParameterLoop *l);

};

/**
 * Receive a sequence of histograms and process them using AnalysisAlgorithms.
 */
class AnalysisProcessor : public AnalysisDataProcessor {
    public:
        AnalysisProcessor();
        AnalysisProcessor(Bookkeeper *b, unsigned ch);
        ~AnalysisProcessor() override;

        void connect(ScanBase *arg_s, ClipBoard<HistogramBase> *arg_input, ClipBoard<HistogramBase> *arg_output, FeedbackClipboard *arg_fb, bool storeInput=false) override {
            scan = arg_s;
            input = arg_input;
            output = arg_output;
            feedback = arg_fb;
            storeInputHisto = storeInput;
        }

        void init() override;
        void run() override;
	    void loadConfig(const json &j);
        void join() override;
        void process() override;
        void process_core();
        void end();

        void addAlgorithm(std::unique_ptr<AnalysisAlgorithm> a);

        bool empty() {return algorithms.empty();}

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
        unsigned id;
        ClipBoard<HistogramBase> *input;
        ClipBoard<HistogramBase> *output;
        FeedbackClipboard *feedback;
        ScanBase *scan;
        std::unique_ptr<std::thread> thread_ptr;
        bool storeInputHisto;
        
        std::vector<std::unique_ptr<AnalysisAlgorithm>> algorithms;
};

#endif
