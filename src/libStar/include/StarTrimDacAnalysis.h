#ifndef STAR_TRIMDAC_ANALYSIS_H
#define STAR_TRIMDAC_ANALYSIS_H

// #################################
// # Author: Olivier Arnaez
// # Email: Olivier.Arnaez at cern.ch
// # Project: Yarr
// # Description: Star TrimDAC Analysis class
// ################################

#include <functional>
#include <vector>

#include "AnalysisAlgorithm.h"

class StarJsonData;

class StarTrimDacAnalysis : public AnalysisAlgorithm {
    public:
        StarTrimDacAnalysis() : AnalysisAlgorithm() {}
        ~StarTrimDacAnalysis() override = default;

        void init(ScanBase *s) override;
        void processHistogram(HistogramBase *h) override;
        void end() override;
        void loadConfig(const json& config) override;


    private:
        std::map<unsigned, std::shared_ptr<const StarJsonData>> m_jDvsTrimDac;
	unsigned par_loopindex;

	unsigned int getChannelMultReachingTarget(const std::map<unsigned, std::map<int, double> > mapThresholdVsTrimDacVsChannelNumber, double target, std::map<unsigned,int> & mapOfTrims) const;

};

#endif

