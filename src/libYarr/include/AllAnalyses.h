#ifndef YARR_ALL_ANALYSIS_ALGORITHMS_H
#define YARR_ALL_ANALYSIS_ALGORITHMS_H

#include "AnalysisAlgorithm.h"

#include <memory>
#include <string>
#include <vector>
#include <functional>

namespace StdDict {
    bool registerAnalysis(std::string name,
                              std::function<std::unique_ptr<AnalysisAlgorithm>()> f);
    std::unique_ptr<AnalysisAlgorithm> getAnalysis(std::string name);

    std::vector<std::string> listAnalyses();
}

#endif
