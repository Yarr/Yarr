// #################################
// # Author: Ryan Quinn
// # Email: ryan.quinn at cern.ch
// # Project: Yarr
// # Description: Star s-curve fit analysis
// ################################

#include <memory>
#include <string>
// #include <unistd.h> // needed to fix linker errors
 
#include "AllAnalyses.h"
#include "ScanLoopInfo.h"
#include "StarChips.h" // IWYU pragma: keep
#include "StarSCurveFitAnalysis.h"
#include "StdHistogrammer.h"

#include "logging.h"

namespace {
    auto alog = logging::make_log("StarSCurveFitAnalysis");
}

namespace {
    bool np_registered =
        StdDict::registerAnalysis("StarSCurveFitAnalysis",
                []() { return std::unique_ptr<AnalysisAlgorithm>(new StarSCurveFitAnalysis());});
}


void StarSCurveFitAnalysis::init(const ScanLoopInfo *s) {
    for (unsigned loopIndex = 0; loopIndex < s->size(); loopIndex++) {
        auto loop = s->getLoop(loopIndex);
        if (isPOILoop(loop)) {
            m_POILoopIndex = loopIndex;
            m_POILoopSteps = (loop->getMax() - loop->getMin()) / loop->getStep();
        }
    }
}


void StarSCurveFitAnalysis::loadConfig(const json &j) {
    if (j.contains("parametersOfInterest")) {
        auto paramsOfInterest = j["parametersOfInterest"];
        if (paramsOfInterest.size() > 1) {
            alog->error("Only one POI is supported, but {} were provided", paramsOfInterest.size());
            exit(1); // I imagine there is a more "proper" way to exit here, not sure if we use exceptions
        }
        m_parametersOfInterest.push_back(paramsOfInterest[0]);
    } else {
        alog->error("No POIs provided");
        exit(1);
    }
}


void StarSCurveFitAnalysis::processHistogram(HistogramBase *h) {
    std::string histogramName = h->getName();
    if (histogramName.find(OccupancyMap::outputName()) != 0) {
        return;
    }
    auto occupancy = dynamic_cast<Histo2d*>(h);

    LoopStatus loopStatus = occupancy->getStat();
    unsigned poiLoopValue = loopStatus.get(m_POILoopIndex);
    LoopStatus::UID id = loopStatus.maskedUniqueID(m_POILoopIndex);

    if (m_sCurves.find(id) == m_sCurves.end()) {
        m_sCurves[id] = std::vector<std::vector<double>>(nRow*nCol, std::vector<double>(m_POILoopSteps, 0.0));
    }

    for (unsigned row = 0; row < nRow; row++) {
        for (unsigned col = 0; col < nCol; col++) {
            int bin = occupancy->binNum(row+1, col+1);
            m_sCurves[id][row*nCol + col][poiLoopValue] = occupancy->getBin(bin);
        }
    }
}


void StarSCurveFitAnalysis::end() {
    // actually do the s-curve fits
}