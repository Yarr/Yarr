#ifndef STAR_NPOINTGAIN_ANALYSIS_H
#define STAR_NPOINTGAIN_ANALYSIS_H

// #################################
// # Author: Ryan Quinn
// # Email: ryan.quinn at cern.ch
// # Project: Yarr
// # Description: Star n-point gain analysis
// ################################

#include <string>
#include <vector>

#include "ScanLoopInfo.h"
#include "StarConversionTools.h"
#include "StdAnalysis.h"

class StarNPointGainAnalysis : public NPointGain {
    public:
        StarNPointGainAnalysis() : NPointGain() {}
        ~StarNPointGainAnalysis() override = default;

        void init(const ScanLoopInfo *s) override;
        void end() override;
        void loadConfig(const json& config) override;

    private:
        // conversion functions for injection and threshold units
        double convertInjectionUnit(double inj) override;
        double convertThresholdUnit(double thr) override;

        // fitting and related functions
        std::vector<double> guessInitialFitParams(const std::vector<double>& thresholds) override;
        std::vector<std::vector<double>> createAverageResponseCurves();

        // 128 strips per side per chip
        static const unsigned s_stripsPerRow = 128;

        // response function and associated parameters
        StarConversionTools* m_conversionTool;

        // member variables
        bool m_convertBVTtomV = true;
        std::string m_thresholdUnit = "mV";
};

#endif