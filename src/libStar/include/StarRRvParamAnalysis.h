#ifndef STARRRVPARAMANALYSIS_H
#define STARRRVPARAMANALYSIS_H

// #################################
// # Author: Jenna Chisholm
// # Email: jenna.lori.chisholm@cern.ch
// # Project: Yarr
// # Description: Star RR vs. Parameter analysis
// ################################

#include <string>
#include <vector>
#include <tuple>
#include "StdAnalysis.h"

class StarRRvParamAnalysis : public AnalysisAlgorithm {
    public:
        StarRRvParamAnalysis() : AnalysisAlgorithm() {}
        ~StarRRvParamAnalysis() override = default;

        void init(const ScanLoopInfo *s) override;
        void processHistogram(HistogramBase *h) override;
        void end() override;
        void loadConfig(const json &config) override;
        void processHCCHistograms(HistogramBase *h);
        void processABCHistograms(HistogramBase *h);
        std::tuple<int, int, int> getHistogramSettings(std::vector<std::vector<int> > map);

    private:
        std::vector<std::string> m_paramNames;
        std::vector<unsigned> m_paramLoopIndices;
        std::vector<unsigned> m_paramMaxBits;
        std::vector<int> m_paramMaxs;
        std::vector<int> m_paramMins;
        std::vector<int> m_paramSteps;
        std::vector<int> m_paramNBins;
        bool readOnly = true;

        std::string m_registerOfInterest;
        std::string m_yAxisTitle;
        
        typedef std::vector<std::vector<int> > RRvParamMap;
        RRvParamMap HCC_map;
        std::vector<RRvParamMap> ABC_maps;
        RRvParamMap HCC_info_map;
        std::vector<RRvParamMap> ABC_info_maps;
        bool filledHCCMap = false;
        bool filledABCMaps = false;
        
};

#endif