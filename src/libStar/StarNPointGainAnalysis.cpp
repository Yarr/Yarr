// #################################
// # Author: Ryan Quinn
// # Email: ryan.quinn at cern.ch
// # Project: Yarr
// # Description: Star N-Point Gain Analysis
// ################################

// #include <numeric>
#include <map>
#include <memory>
#include <string>
#include <tuple>
#include <utility>
#include <vector>
#include <unistd.h> // needed to fix linker errors
 
#include "AllAnalyses.h"
#include "Histo1d.h"
#include "Histo2d.h"
#include "Histo3d.h"
#include "ScanLoopInfo.h"
#include "StarChips.h"
#include "StarConversionTools.h"
#include "StarNPointGainAnalysis.h"
#include "StdAnalysis.h"

#include "lmcurve.h"
#include "logging.h"

namespace {
    auto alog = logging::make_log("StarNPointGainAnalysis");
}

namespace {
    bool np_registered =
        StdDict::registerAnalysis("StarNPointGainAnalysis",
                []() { return std::unique_ptr<AnalysisAlgorithm>(new StarNPointGainAnalysis());});
}


void StarNPointGainAnalysis::init(const ScanLoopInfo *s) {
    NPointGain::init(s);

    // initialize conversion tool for unit conversions
    m_conversionTool = &dynamic_cast<StarCfg*>(feCfg)->getStarConversion();
    m_conversionTool->setResponseFunction(m_respFuncName);
}


double StarNPointGainAnalysis::convertInjectionUnit(double inj) {
    return m_conversionTool->convertBCALtofC(inj);
}

double StarNPointGainAnalysis::convertThresholdUnit(double thr) {
    return m_convertBVTtomV ? m_conversionTool->convertBVTtomV(thr) : thr;
}

void StarNPointGainAnalysis::loadConfig(const json &j) {
    NPointGain::loadConfig(j);

    if (j.contains("convertBVTtomV")) {
        m_convertBVTtomV = j["convertBVTtomV"];
        m_thresholdUnit = m_convertBVTtomV ? "mV" : "BVT";
    }
}


std::vector<std::vector<double>> StarNPointGainAnalysis::createAverageResponseCurves() {

    unsigned nChips = nCol / s_stripsPerRow;
    std::vector<std::vector<double>> averages(nChips, std::vector<double>(m_injections.size()));
    std::vector<std::vector<unsigned>> sizes(nChips, std::vector<unsigned>(m_injections.size()));

    for (const auto& inj : m_injections) {
        for (unsigned col = 0; col < nCol; col++) {
            unsigned chip = col / s_stripsPerRow;
            for (unsigned row = 0; row < nRow; row++) {
                auto thr = m_thresholdMap[inj][col][row];
                auto size = sizes[chip][inj];
                averages[chip][inj] += (size * averages[chip][inj] * thr) / (size + 1);
                sizes[chip][inj]++;
            }
        }
    }

    return averages;
}


void StarNPointGainAnalysis::end() {
    NPointGain::end();

    auto respCurvesByChip = createAverageResponseCurves();
    for (unsigned chip = 0; chip < (nCol/s_stripsPerRow); chip++) {
        // fit chip-avg response curve and fill output configuration
        auto thresholds = respCurvesByChip[chip];
        std::vector<double> fitParams = guessInitialFitParams(thresholds);
        fitResponseCurve(thresholds, fitParams);
        m_conversionTool->setResponseParameters(fitParams, chip);
    }
}


std::vector<double> StarNPointGainAnalysis::guessInitialFitParams(const std::vector<double>& thresholds)
{
    std::vector<double> fitParams;

    if (m_respFuncName == "exponential") {
        // special case for our exponential fit
        fitParams.push_back(900.);
        fitParams.push_back(6.);
        fitParams.push_back(-400.);
        if (!m_convertBVTtomV) {
            for (unsigned i = 0; i < fitParams.size(); i++) {
                fitParams[i] /= 2;
            }
        }
    } else {
        // linear + polynomial already done in base class
        fitParams = NPointGain::guessInitialFitParams(thresholds);
    }

    return fitParams;
}
