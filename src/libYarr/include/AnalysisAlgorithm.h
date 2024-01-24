#ifndef YARR_ANALYSIS_ALGORITHM_H
#define YARR_ANALYSIS_ALGORITHM_H

#include <thread>

#include "AnalysisDataProcessor.h"
#include "FeedbackBase.h"
#include "HistogramBase.h"
#include "ScanLoopInfo.h"

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
        
        void setConfig (FrontEndCfg *c) { feCfg = c;}
        void setId (unsigned uid) {id = uid;}

        virtual void setParams(int target_tot, int target_charge) {}

        void connect(ClipBoard<HistogramBase> *out, FeedbackClipboard *fb) {
            output = out;
            feedback = fb;
        }
        virtual void init(const ScanLoopInfo *s) {}
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
        FrontEndCfg *feCfg;
        unsigned id;
        ClipBoard<HistogramBase> *output;
        FeedbackClipboard *feedback;
        bool make_mask;
        unsigned nCol, nRow;

        std::vector<std::string> m_parametersOfInterest;
        bool isPOILoop(const LoopActionBaseInfo *l);

};

/**
 * Receive a sequence of histograms and process them using AnalysisAlgorithms.
 */
class AnalysisProcessor : public AnalysisDataProcessor {
    public:
        AnalysisProcessor();
        AnalysisProcessor(unsigned ch);
        ~AnalysisProcessor() override;

        void connect(const ScanLoopInfo *arg_s, ClipBoard<HistogramBase> *arg_input, ClipBoard<HistogramBase> *arg_output, FeedbackClipboard *arg_fb, bool storeInput=false) override {
            scan_info = arg_s;
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
        unsigned id;
        ClipBoard<HistogramBase> *input;
        ClipBoard<HistogramBase> *output;
        FeedbackClipboard *feedback;
        const ScanLoopInfo *scan_info;
        std::unique_ptr<std::thread> thread_ptr;
        bool storeInputHisto;
        
        std::vector<std::unique_ptr<AnalysisAlgorithm>> algorithms;
};

#endif
