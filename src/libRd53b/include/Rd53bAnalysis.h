#ifndef RD53B_ANALYSIS_H
#define RD53B_ANALYSIS_H

// std/stl
#include <vector>

// yarr
#include "ScanBase.h"
#include "AllStdActions.h"
#include "AnalysisAlgorithm.h"
#include "Histo1d.h"
#include "Histo2d.h"
class HistogramBase;

class FrontEndScopeAnalysis : public AnalysisAlgorithm {

    public :
        FrontEndScopeAnalysis() : AnalysisAlgorithm() {};
        ~FrontEndScopeAnalysis() {}

        void loadConfig(json& config);
        void init(ScanBase* s);
        void processHistogram(HistogramBase* h);
        void end();

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

class PrecisionTimingAnalysis : public AnalysisAlgorithm {

    public :
        PrecisionTimingAnalysis() : AnalysisAlgorithm() {};
        ~PrecisionTimingAnalysis() {};

        void init(ScanBase *s);
        void processHistogram(HistogramBase* h);
        void end() {}
        void loadConfig(json& config);

    private :

        std::vector<unsigned> m_loops;
        std::vector<unsigned> m_loopMax;
        unsigned m_n_count;
        unsigned m_injections;

        std::map<unsigned, std::unique_ptr<Histo2d>> h_occMaps;
        std::map<unsigned, unsigned> occ_count;

        std::map<unsigned, std::unique_ptr<Histo1d>> h_ptotDists;
        std::map<unsigned, unsigned> ptot_dist_count;

        std::map<unsigned, std::unique_ptr<Histo1d>> h_ptoaDists;
        std::map<unsigned, unsigned> ptoa_dist_count;

        std::map<unsigned, std::unique_ptr<Histo2d>> h_ptotMaps;
        std::map<unsigned, unsigned> ptot_count;

        std::map<unsigned, std::unique_ptr<Histo2d>> h_ptot2Maps;
        std::map<unsigned, unsigned> ptot2_count;

        std::map<unsigned, std::unique_ptr<Histo2d>> h_ptoaMaps;
        std::map<unsigned, unsigned> ptoa_count;

        std::map<unsigned, std::unique_ptr<Histo2d>> h_ptoa2Maps;
        std::map<unsigned, unsigned> ptoa2_count;

};

#endif
