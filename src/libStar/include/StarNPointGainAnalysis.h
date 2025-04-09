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
        /// @brief Convert injection units from BCAL to fC
        /// @param inj Charge injection in BCAL units
        /// @return Charge injection in fC
        double convertInjectionUnit(double inj) override;

        /// @brief Convert threshold units from BVT to mV
        /// This conversion is applied to both response and output noise.
        /// @param thr Threshold in BVT units
        /// @return Threshold in mV
        double convertThresholdUnit(double thr) override;

        /// @brief Convert input noise units from fC to ENC
        /// @param noise Input noise in fC
        /// @return Input noise in ENC
        double convertInputNoiseUnit(double noise) override;

        /// @brief Guess the initial fit parameters for the response curve fit
        /// @param thresholds Vector of response values
        /// @return Initial guess for fit parameters
        std::vector<double> guessInitialFitParams(const std::vector<double>& thresholds) override;

        /// @brief Create an averaged response curve for for each chip
        /// @return Vector of vectors of averaged response curves (index by [chip][injection])
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