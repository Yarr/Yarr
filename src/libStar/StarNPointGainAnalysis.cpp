// #################################
// # Author: Ryan Quinn
// # Email: ryan.quinn at cern.ch
// # Project: Yarr
// # Description: Star N-Point Gain Analysis
// ################################

#include <map>
#include <memory>
#include <string>
#include <vector>
#include <unistd.h> // needed to fix linker errors
 
#include "AllAnalyses.h"
#include "ScanLoopInfo.h"
#include "StarChips.h" // IWYU pragma: keep
#include "StarConstants.h"
#include "StarConversionTools.h"
#include "StarNPointGainAnalysis.h"
#include "StdAnalysis.h"

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

double StarNPointGainAnalysis::convertInputNoiseUnit(double noise) {
    return m_conversionTool->convertfCtoENC(noise);
}

void StarNPointGainAnalysis::loadConfig(const json &j) {
    NPointGain::loadConfig(j);

    if (j.contains("convertBVTtomV")) {
        m_convertBVTtomV = j["convertBVTtomV"];
        m_thresholdUnit = m_convertBVTtomV ? "mV" : "BVT";
    }
}


std::vector<std::vector<double>> StarNPointGainAnalysis::createAverageResponseCurves() {

    unsigned nChips = nCol / Star::StripsPerABCRow;
    unsigned nInj = m_injections.size();
    std::vector<std::vector<double>> averages(nChips, std::vector<double>(nInj));

    for (unsigned injIdx = 0; injIdx < nInj; injIdx++) {
        double inj = m_injections[injIdx];
        for (unsigned chip = 0; chip < nChips; chip++) {
            double sum = 0.;
            for (unsigned row = 0; row < nRow; row++) {
                for (unsigned strip = 0; strip < Star::StripsPerABCRow; strip++) {
                    unsigned col = (chip * Star::StripsPerABCRow) + strip;
                    sum += m_thresholdMap[inj][col][row];
                }
            }

            averages[chip][injIdx] = sum / (nRow*Star::StripsPerABCRow);
        }
    }

    return averages;
}


void StarNPointGainAnalysis::end() {
    NPointGain::end();

    auto respCurvesByChip = createAverageResponseCurves();
    for (unsigned chip = 0; chip < (nCol/Star::StripsPerABCRow); chip++) {
        // fit chip-avg response curve and fill output configuration
        const auto& thresholds = respCurvesByChip[chip];
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
