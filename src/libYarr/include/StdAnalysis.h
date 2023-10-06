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

// Need size to make unique_ptr destructors
#include "Histo1d.h"
#include "Histo2d.h"
#include "Histo3d.h"

class OccupancyAnalysis : public AnalysisAlgorithm {
    public:
        OccupancyAnalysis() : AnalysisAlgorithm() {createMask = true; LowThr = 0.0; HighThr = 0.0;}
        ~OccupancyAnalysis() override = default;

        void init(const ScanLoopInfo *s) override;
        void processHistogram(HistogramBase *h) override;
        void end() override {}
        void loadConfig(const json &config) override;
    private:
        std::vector<unsigned> loops;
        std::vector<unsigned> loopMax;
        bool createMask;
        unsigned n_count;
        unsigned injections;
	double LowThr, HighThr;
        std::map<unsigned, std::unique_ptr<Histo2d>> occMaps;
        std::map<unsigned, unsigned> innerCnt;
};

class HistogramArchiver : public AnalysisAlgorithm {
    public:
        HistogramArchiver() = default;
        ~HistogramArchiver() override = default;

        void init(const ScanLoopInfo *s) override;
        void processHistogram(HistogramBase *h) override;
        void end() override {}
        void loadConfig(const json &config) override;

        void setOutputDirectory(std::string dir);
    private:
        std::string output_dir;
};

class TotAnalysis : public AnalysisAlgorithm {
    public:
        TotAnalysis() : AnalysisAlgorithm() {
            tot_bins_n = 16;
            tot_bins_x_lo = 0;
            tot_bins_x_hi = 16;
            tot_unit = "BC";
            tot_sigma_bins_n = 101;
            tot_sigma_bins_x_lo = -0.05;
            tot_sigma_bins_x_hi = 1.05;
        }
        ~TotAnalysis() override = default;

        void init(const ScanLoopInfo *s) override;
        void processHistogram(HistogramBase *h) override;
        void end() override;
        void loadConfig(const json &config) override;

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
	std::unique_ptr<Histo2d> pixelTotMap;
	std::unique_ptr<Histo1d> RMSTotVsCharge;

        // histogram configuration for ToT distributions
        unsigned tot_bins_n;
        float tot_bins_x_lo;
        float tot_bins_x_hi;
        std::string tot_unit;

        // histogram configuration for ToT sigma distributions
        unsigned tot_sigma_bins_n;
        float tot_sigma_bins_x_lo;
        float tot_sigma_bins_x_hi;
};

class NPointGain : public AnalysisAlgorithm {
    public:
        NPointGain() : AnalysisAlgorithm() {}
        ~NPointGain() override = default;

        void init(const ScanLoopInfo *s) override;
        void processHistogram(HistogramBase *h) override;
        void end() override;
        void loadConfig(const json& config) override;

        bool requireDependency() override {return !m_skipDependencyCheck;}

    private:
        std::unique_ptr<Histo1d> respCurve;

        std::vector<double> inj;
        std::vector<double> inj_err;
        std::vector<double> thr;
        std::vector<double> thr_err;

        unsigned par_loopindex;
        unsigned par_min;
        unsigned par_max;
        unsigned par_step;

	bool m_skipDependencyCheck=false;
};

class ScurveFitter : public AnalysisAlgorithm {
    public:
        ScurveFitter() : AnalysisAlgorithm() {}
        ~ScurveFitter() override = default;

        void init(const ScanLoopInfo *s) override;
        void processHistogram(HistogramBase *h) override;
        void end() override;
        void loadConfig(const json &config) override;
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
        bool reverse = false;

        bool m_dumpDebugScurvePlots=false;
};

class OccGlobalThresholdTune : public AnalysisAlgorithm {
    public:
        OccGlobalThresholdTune() : AnalysisAlgorithm()  {}
        ~OccGlobalThresholdTune() override = default;

        void init(const ScanLoopInfo *s) override;
        void processHistogram(HistogramBase *h) override;
        void end() override {}
        void loadConfig(const json &config) override{}
    private:
        std::vector<unsigned> loops;
        std::vector<unsigned> loopMax;
        unsigned n_count;
        std::map<unsigned, std::unique_ptr<Histo2d>> occMaps;
        std::map<unsigned, std::unique_ptr<Histo1d>> occDists;
        std::map<unsigned, unsigned> innerCnt;
        unsigned injections;
        std::unique_ptr<GlobalFeedbackSender> fb;
};

class GlobalPreampTune : public AnalysisAlgorithm {
    public:
        GlobalPreampTune() : AnalysisAlgorithm()  {}
        ~GlobalPreampTune() override = default;

        void init(const ScanLoopInfo *s) override;
        void processHistogram(HistogramBase *h) override;
        void end() override {}
        void loadConfig(const json &config) override{}

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
        ~OccPixelThresholdTune() override = default;

        void init(const ScanLoopInfo *s) override;
        void processHistogram(HistogramBase *h) override;
        void end() override {}
        void loadConfig(const json &config) override;

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
        ~L1Analysis() override = default;

        void init(const ScanLoopInfo *s) override;
        void processHistogram(HistogramBase *h) override;
        void end() override;
        void loadConfig(const json &config) override{}
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
        ~TagAnalysis() override = default;

        void init(const ScanLoopInfo *s) override;
        void processHistogram(HistogramBase *h) override;
        void end() override;
        void loadConfig(const json &config) override{}
    private:
        std::vector<unsigned> loops;
        std::vector<unsigned> loopMax;
        unsigned n_count;
        unsigned injections;
        std::map<unsigned, std::unique_ptr<Histo1d>> tagHistos;
        std::map<unsigned, std::unique_ptr<Histo2d>> tagMaps;
        std::map<unsigned, std::unique_ptr<Histo2d>> occMaps;
        std::map<unsigned, unsigned> tagDistInnerCnt;
        std::map<unsigned, unsigned> tagMapInnerCnt;
        std::map<unsigned, unsigned> occInnerCnt;

};

class TotDistPlotter : public AnalysisAlgorithm {
    public:
        TotDistPlotter() : AnalysisAlgorithm() {}
        ~TotDistPlotter() override = default;

        void init(const ScanLoopInfo *s) override;
        void processHistogram(HistogramBase *h) override;
        void end() override {}
        void loadConfig(const json &config) override{}
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
        NoiseAnalysis() : AnalysisAlgorithm() {
            createMask = true;
            noiseThr = 1e-6;
        }
        ~NoiseAnalysis() override = default;

        void init(const ScanLoopInfo *s) override;
        void processHistogram(HistogramBase *h) override;
        void end() override;
        void loadConfig(const json &config) override;
    private:
        unsigned n_trigger;
        std::unique_ptr<Histo2d> occ, tot;
        std::unique_ptr<Histo1d> tag;      
        bool createMask;
        double noiseThr;
};

class NoiseTuning : public AnalysisAlgorithm {
    public:
        NoiseTuning() : AnalysisAlgorithm() {}
        ~NoiseTuning() override = default;

        void init(const ScanLoopInfo *s) override;
        void processHistogram(HistogramBase *h) override;
        void end() override;
        void loadConfig(const json &config) override{}
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
        ~DelayAnalysis() override = default;

        void init(const ScanLoopInfo *s) override;
        void processHistogram(HistogramBase *h) override;
        void end() override;
        void loadConfig(const json &config) override{}

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
        ~ParameterAnalysis() override = default;;

        void init(const ScanLoopInfo *s) override;
        void processHistogram(HistogramBase *h) override;
        void end() override;
	void loadConfig(const json &config) override {}
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
