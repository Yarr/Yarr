#ifndef RD53B_ANALYSIS_H
#define RD53B_ANALYSIS_H

// std/stl
#include <vector>

// yarr
#include "ScanLoopInfo.h"
#include "AllStdActions.h"
#include "AnalysisAlgorithm.h"
#include "Histo1d.h"
#include "Histo2d.h"
class HistogramBase;

class FrontEndScopeAnalysis : public AnalysisAlgorithm {

    public :
        FrontEndScopeAnalysis() : AnalysisAlgorithm() {};
        ~FrontEndScopeAnalysis() override = default;

        void loadConfig(const json &config) override;
        void init(const ScanLoopInfo* s) override;
        void processHistogram(HistogramBase* h) override;
        void end() override;

    private:

        std::vector<unsigned> m_loops;
        std::vector<unsigned> m_loopMax;
        unsigned m_n_count;
        double m_injections;

        bool m_has_threshold_loop;
        unsigned m_threshold_min;
        unsigned m_threshold_max;
        unsigned m_threshold_step;
        unsigned m_threshold_bins;

        std::map<unsigned, std::unique_ptr<Histo2d>> h_occMaps;
        std::map<unsigned, unsigned> m_occ_count;

        std::map<unsigned, std::unique_ptr<Histo2d>> h_ptotMaps;
        std::map<unsigned, unsigned> m_ptot_count;

        std::map<unsigned, std::unique_ptr<Histo2d>> h_ptoaMaps;
        std::map<unsigned, unsigned> m_ptoa_count;

        std::unique_ptr<Histo2d> h_pulseShapeMap;
        bool m_doPulseShapeMap = true;
        float m_pulseShape_xlo = 100.0;
        float m_pulseShape_xhi = 400.0;
        unsigned m_pulseShape_nxbins = 300;
        float m_pulseShape_ylo = 0.0;
        float m_pulseShape_yhi = 500.0;
        unsigned m_pulseShape_nybins = 250;

        bool m_exclude_LRCols = true;
};

class ToaAnalysis : public AnalysisAlgorithm {

    public :
        ToaAnalysis() : AnalysisAlgorithm() {};
        ~ToaAnalysis() override = default;;

        void init(const ScanLoopInfo *s) override;
        void processHistogram(HistogramBase* h) override;
        void end() override;
        void loadConfig(const json &config) override;

    private :

        std::vector<unsigned> m_loops;
        std::vector<unsigned> m_loopMax;
        unsigned m_n_count;
        unsigned m_injections;

        std::map<unsigned, std::unique_ptr<Histo2d>> h_occMaps;
        std::map<unsigned, unsigned> occ_count;

        std::map<unsigned, std::unique_ptr<Histo2d>> h_ptoaMaps;
        std::map<unsigned, unsigned> ptoa_count;

        std::map<unsigned, std::unique_ptr<Histo2d>> h_ptoa2Maps;
        std::map<unsigned, unsigned> ptoa2_count;

        std::map<unsigned, std::unique_ptr<Histo1d>> h_ptoaDists;
        std::map<unsigned, unsigned> ptoa_dist_count;

        // histogram configuration for ToA distributions
        unsigned toa_bins_n = 33;
        float toa_bins_x_lo = -0.5;
        float toa_bins_x_hi = 32.5;

        // histogram configuration for ToA sigma distributions
        unsigned toa_sigma_bins_n = 101;
        float toa_sigma_bins_x_lo = -0.05;
        float toa_sigma_bins_x_hi = 5.05;

        std::string toa_unit = "1.5625 ns";

        // has ∆Vcal loop (for ToA vs charge)
        bool m_hasVcalLoop = false;
        unsigned m_vcalMax;
        unsigned m_vcalMin;
        unsigned m_vcalStep;
        unsigned m_vcalNBins;
        std::unique_ptr<Histo2d> h_chargeVsToaMap;


};

#endif
