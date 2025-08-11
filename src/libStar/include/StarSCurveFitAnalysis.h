#ifndef STAR_SCURVEFIT_ANALYSIS_H
#define STAR_SCURVEFIT_ANALYSIS_H

// #################################
// # Author: Ryan Quinn
// # Email: ryan.quinn at cern.ch
// # Project: Yarr
// # Description: Star s-curve fit analysis
// ################################

#include "AnalysisAlgorithm.h"
#include "ScanLoopInfo.h"

class StarSCurveFitAnalysis : public AnalysisAlgorithm {
    public:
        StarSCurveFitAnalysis() : AnalysisAlgorithm() {}
        ~StarSCurveFitAnalysis() override = default;

        void init(const ScanLoopInfo *s) override;
        void processHistogram(HistogramBase *h) override;
        void end() override;
        void loadConfig(const json& config) override;

    private:
        unsigned m_POILoopIndex = 0;
        unsigned m_POILoopSteps = 0;

        std::map<LoopStatus::UID, std::vector<std::vector<double>>> m_sCurves;
};

#endif