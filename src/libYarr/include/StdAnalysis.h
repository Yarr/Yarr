#ifndef STD_ANALYSIS_H
#define STD_ANALYSIS_H

// #################################
// # Author: Timon Heim
// # Email: timon.heim at cern.ch
// # Project: Yarr
// # Description: Analysis Base class
// ################################

#include <vector>

#include "AnalysisAlgorithm.h"

class HistogramBase;
class Histo1d;
class Histo2d;
class Histo3d;

class OccupancyAnalysis : public AnalysisAlgorithm {
    public:
        OccupancyAnalysis() : AnalysisAlgorithm() {}
        ~OccupancyAnalysis() {}

        void init(ScanBase *s);
        void processHistogram(HistogramBase *h);
        void end() {}
        void loadConfig(json &config);
    private:
        std::vector<unsigned> loops;
        std::vector<unsigned> loopMax;
        bool createMask;
        unsigned n_count;
        unsigned injections;
        std::map<unsigned, std::unique_ptr<Histo2d>> occMaps;
        std::map<unsigned, unsigned> innerCnt;
};

class TotAnalysis : public AnalysisAlgorithm {
    public:
        TotAnalysis() : AnalysisAlgorithm() {}
        ~TotAnalysis() {}

        void init(ScanBase *s);
        void processHistogram(HistogramBase *h);
        void end();
        void loadConfig(json &config);

    private:
        std::vector<unsigned> loops;
        std::vector<unsigned> loopMax;
        unsigned n_count;
        double injections;
        std::map<unsigned, std::unique_ptr<Histo2d>> occMaps;
        std::map<unsigned, unsigned> occInnerCnt;
        std::map<unsigned, std::unique_ptr<Histo2d>> totMaps;
        std::map<unsigned, unsigned> totInnerCnt;
        std::map<unsigned, std::unique_ptr<Histo2d>> tot2Maps;
        std::map<unsigned, unsigned> tot2InnerCnt;
        std::unique_ptr<GlobalFeedbackSender> globalFb;
        std::unique_ptr<PixelFeedbackSender> pixelFb;
        bool useScap;
        bool useLcap;
        bool hasVcalLoop;
        unsigned vcalMin;
        unsigned vcalMax;
        unsigned vcalStep;
        unsigned vcalBins;
        std::unique_ptr<Histo2d> chargeVsTotMap;

        // histogram configuration for ToT distributions
        unsigned tot_bins_n = 16;
        float tot_bins_x_lo = 0;
        float tot_bins_x_hi = 16;
        std::string tot_unit = "BC";

        // histogram configuration for ToT sigma distributions
        unsigned tot_sigma_bins_n = 101;
        float tot_sigma_bins_x_lo = -0.05;
        float tot_sigma_bins_x_hi = 1.05;
};

class ScurveFitter : public AnalysisAlgorithm {
    public:
        ScurveFitter() : AnalysisAlgorithm() {}
        ~ScurveFitter() {}

        void init(ScanBase *s);
        void processHistogram(HistogramBase *h);
        void end();
        void loadConfig(json &config){}
    private:
        unsigned vcalLoop;
        unsigned vcalMin;
        unsigned vcalMax;
        unsigned vcalStep;
        unsigned vcalBins;
        unsigned n_count;
        unsigned injections;
        unsigned cnt;
        unsigned n_failedfit;

        std::vector<double> x;
        std::vector<unsigned> loops;
        std::vector<unsigned> loopMax;

        std::map<unsigned, std::unique_ptr<Histo1d>> histos;
        std::map<unsigned, std::unique_ptr<Histo2d>> thrMap;
        std::map<unsigned, std::unique_ptr<Histo1d>> thrDist;
        std::map<unsigned, std::unique_ptr<Histo2d>> sigMap;
        std::map<unsigned, std::unique_ptr<Histo1d>> sigDist;
        std::map<unsigned, std::unique_ptr<Histo1d>> chiDist;
        std::map<unsigned, std::unique_ptr<Histo1d>> timeDist;

        std::map<unsigned, std::unique_ptr<Histo2d>> chi2Map;   
        std::map<unsigned, std::unique_ptr<Histo2d>> statusMap; 
        std::map<unsigned, std::unique_ptr<Histo1d>> statusDist;

        std::unique_ptr<PixelFeedbackSender> fb;
        std::map<unsigned, std::unique_ptr<Histo2d>> step;
        std::map<unsigned, std::unique_ptr<Histo2d>> deltaThr;
        unsigned prevOuter;
        double thrTarget;

        std::map<unsigned, unsigned> innerCnt;
        std::map<unsigned, unsigned> medCnt;
        std::map<unsigned, unsigned> vcalCnt;
        bool useScap;
        bool useLcap;
};

class OccGlobalThresholdTune : public AnalysisAlgorithm {
    public:
        OccGlobalThresholdTune() : AnalysisAlgorithm()  {}
        ~OccGlobalThresholdTune() {}

        void init(ScanBase *s);
        void processHistogram(HistogramBase *h);
        void end() {}
        void loadConfig(json &config){}
    private:
        std::vector<unsigned> loops;
        std::vector<unsigned> loopMax;
        unsigned n_count;
        std::map<unsigned, std::unique_ptr<Histo2d>> occMaps;
        std::map<unsigned, std::unique_ptr<Histo1d>> occDists;
        std::map<unsigned, unsigned> innerCnt;
        unsigned injections;
        std::unique_ptr<GlobalFeedbackSender> fb;
        LoopActionBase *lb;

};

class GlobalPreampTune : public AnalysisAlgorithm {
    public:
        GlobalPreampTune() : AnalysisAlgorithm()  {}
        ~GlobalPreampTune() {}

        void init(ScanBase *s);
        void processHistogram(HistogramBase *h);
        void end() {}
        void loadConfig(json &config){}

    private:
        std::vector<unsigned> loops;
        std::vector<unsigned> loopMax;
        unsigned n_count;
        std::map<unsigned, std::unique_ptr<Histo2d>> occMaps;
        std::map<unsigned, std::unique_ptr<Histo2d>> totMaps;
        std::map<unsigned, std::unique_ptr<Histo1d>> occDists;
        std::map<unsigned, unsigned> innerCnt;
        unsigned injections;
        std::unique_ptr<GlobalFeedbackSender> fb;

};

class OccPixelThresholdTune : public AnalysisAlgorithm {
    public:
        OccPixelThresholdTune() : AnalysisAlgorithm()  {
            m_occLowCut = 0.3;
            m_occHighCut = 0.7;
        }
        ~OccPixelThresholdTune() {}

        void init(ScanBase *s);
        void processHistogram(HistogramBase *h);
        void end() {}
        void loadConfig(json &config);

    private:
        std::vector<unsigned> loops;
        std::vector<unsigned> loopMax;
        unsigned n_count;
        float m_occLowCut;
        float m_occHighCut;
        std::map<unsigned, std::unique_ptr<Histo2d>> occMaps;
        std::map<unsigned, unsigned> innerCnt;
        unsigned injections;
        std::unique_ptr<PixelFeedbackSender> fb;

};

class L1Analysis : public AnalysisAlgorithm {
    public:
        L1Analysis() : AnalysisAlgorithm() {}
        ~L1Analysis() {}

        void init(ScanBase *s);
        void processHistogram(HistogramBase *h);
        void end();
        void loadConfig(json &config){}
    private:
        std::vector<unsigned> loops;
        std::vector<unsigned> loopMax;
        unsigned n_count;
        unsigned injections;
        std::map<unsigned, std::unique_ptr<Histo1d>> l1Histos;
        std::map<unsigned, unsigned> innerCnt;

};

class TagAnalysis : public AnalysisAlgorithm {
    public:
        TagAnalysis() : AnalysisAlgorithm() {}
        ~TagAnalysis() {}

        void init(ScanBase *s);
        void processHistogram(HistogramBase *h);
        void end();
        void loadConfig(json &config){}
    private:
        std::vector<unsigned> loops;
        std::vector<unsigned> loopMax;
        unsigned n_count;
        unsigned injections;
        std::map<unsigned, std::unique_ptr<Histo1d>> tagHistos;
        std::map<unsigned, unsigned> innerCnt;

};

class TotDistPlotter : public AnalysisAlgorithm {
    public:
        TotDistPlotter() : AnalysisAlgorithm() {}
        ~TotDistPlotter() {}

        void init(ScanBase *s);
        void processHistogram(HistogramBase *h);
        void end() {}
        void loadConfig(json &config){}
    private:
        std::vector<unsigned> loops;
        std::vector<unsigned> loopMax;
        unsigned n_count;
        unsigned injections;
        std::map<unsigned, std::unique_ptr<Histo1d>> tot;
        std::map<unsigned, unsigned> innerCnt;
};

class NoiseAnalysis : public AnalysisAlgorithm {
    public:
        NoiseAnalysis() : AnalysisAlgorithm() {}
        ~NoiseAnalysis() {}

        void init(ScanBase *s);
        void processHistogram(HistogramBase *h);
        void end();
        void loadConfig(json &config);
    private:
        unsigned n_trigger;
        std::unique_ptr<Histo2d> occ;
        bool createMask;
};

class NoiseTuning : public AnalysisAlgorithm {
    public:
        NoiseTuning() : AnalysisAlgorithm() {}
        ~NoiseTuning() {}

        void init(ScanBase *s);
        void processHistogram(HistogramBase *h);
        void end();
        void loadConfig(json &config){}
    private:
        std::vector<unsigned> loops;
        std::vector<unsigned> loopMax;
        unsigned n_count;
        std::map<unsigned, std::unique_ptr<Histo2d>> occMaps;
        std::map<unsigned, unsigned> innerCnt;
        std::unique_ptr<GlobalFeedbackSender> globalFb;
        std::unique_ptr<PixelFeedbackSender> pixelFb;
};

class DelayAnalysis : public AnalysisAlgorithm {
    public:
        DelayAnalysis() : AnalysisAlgorithm() {}
        ~DelayAnalysis() {}

        void init(ScanBase *s);
        void processHistogram(HistogramBase *h);
        void end();
        void loadConfig(json &config){}

    private:
        std::vector<unsigned> loops;
        std::vector<unsigned> loopMax;
        unsigned n_count;
        std::map<unsigned, std::unique_ptr<Histo3d>> occMaps;
        std::map<unsigned, std::unique_ptr<Histo1d>> histos;
        std::map<unsigned, unsigned> innerCnt;
        unsigned injections;

        std::unique_ptr<Histo2d> delayMap;
        std::unique_ptr<Histo2d> rmsMap;

        unsigned delayLoop;
        unsigned delayMin;
        unsigned delayMax;
        unsigned delayStep;
        unsigned count;
};

class ParameterAnalysis : public AnalysisAlgorithm {
    public:
        ParameterAnalysis() : AnalysisAlgorithm() {};
        ~ParameterAnalysis() {};

        void init(ScanBase *s);
        void processHistogram(HistogramBase *h);
        void end();
	void loadConfig(json &config) {}
    private:
        std::vector<unsigned> loops;
        std::vector<unsigned> loopMax;
        unsigned n_count;
        unsigned injections;
	unsigned paramLoopNo;
	unsigned paramMin;
	unsigned paramMax;
	unsigned paramStep;
        unsigned paramBins;
	unsigned count;
        std::string paramName;
        std::map<unsigned, std::unique_ptr<Histo2d>> occMaps;
        std::map<unsigned, std::unique_ptr<Histo2d>> paramCurves;
        std::map<unsigned, std::unique_ptr<Histo2d>> paramMaps;

};

#endif
