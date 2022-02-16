// #################################
// # Author: Olivier Arnaez
// # Email: Olivier.Arnaez at cern.ch
// # Project: Yarr
// # Description: TrimDac Analysis class for Star
// ################################

#include "StarTrimDacAnalysis.h"
#include "AllAnalyses.h"
#include "StarJsonData.h"
#include "Histo1d.h"
#include "GraphErrors.h"

#include "logging.h"

namespace {
    auto alog = logging::make_log("StarTrimDacAnalysis");
}

namespace {
    bool oa_registered =
      StdDict::registerAnalysis("StarTrimDacAnalysis",
                                []() { return std::unique_ptr<AnalysisAlgorithm>(new StarTrimDacAnalysis());});
}

void StarTrimDacAnalysis::init(ScanBase *s) {
    for (unsigned n=0; n<s->size(); n++) {
        std::shared_ptr<LoopActionBase> l = s->getLoop(n);
        if ( l->isParameterLoop() && isPOILoop(dynamic_cast<StdParameterLoop*>(l.get())) ) {
            par_loopindex = n;
/*            par_min = l->getMin();
            par_max = l->getMax();
            par_step = l->getStep();*/
            break;
        }
    }
}

void StarTrimDacAnalysis::processHistogram(HistogramBase *h) {
    // Pick the threshold map based on histogram names
    // Target string: "ThresholdMap-<parameter>"
    std::string hname = h->getName();
    if (h->getName().find("JsonData_StarThresholdResult")!=0)
        return;

    //Getting the input JsonData
    const StarJsonData* jd = new StarJsonData(*((JsonData*)h));
    // Get the scan parameter value (TrimDAC)
    unsigned par = h->getStat().get(par_loopindex);

    m_jDvsTrimDac[par].reset(jd);
}

void StarTrimDacAnalysis::end() {
        if (!m_jDvsTrimDac.size())
                return;

        //Creating a new LoopStatus "agregating" the POI in order to be used in outputs
        std::vector<unsigned> newLSstat;
        std::vector<LoopStyle> newLSstyle;
        std::shared_ptr<const StarJsonData> spFirstJD = m_jDvsTrimDac.begin()->second;
        const StarJsonData* firstJD = spFirstJD.get();
        LoopStatus lStat = firstJD->getStat();
        for (unsigned n=0; n<lStat.size(); n++)
                if (n!=par_loopindex) {
                        newLSstat.push_back( lStat.get(n) );
                        newLSstyle.push_back( (LoopStyle) lStat.getStyle(n) );
                }
        LoopStatus newLoopStatus(std::move(newLSstat), newLSstyle);
        alog->debug("creating StarJsonData object to store results");
        StarJsonData * outJD = new StarJsonData("JsonData_StarTrimDACResult", newLoopStatus);
        outJD->setJsonDataType("JsonData_StarTrimDACResult");
        std::unique_ptr<StarJsonData> upJD;
        upJD.reset(outJD);

        //Doing here the search for the best target Threshold looping all m_jDvsTrimDac (i.e. finding the position "threshold_max" of maximum in the distribution of Thresholds having the largest peak)
        //Storing all input results in big map that will later be used to search for the best TrimDac
        std::map<unsigned, std::map<int, double> > mapThresholdVsTrimDacVsChannelNumber;
        //And list of potential targets
        std::vector<double> listThresholds;
        for (unsigned int iChip=0; iChip<(nCol/128); iChip++) {
                //Will also fill some debugging plots on the way
                GraphErrors* grTrimDacVsThresholdForChip = new GraphErrors("TrimDacVsThreshold_Chip" + std::to_string(iChip));
                grTrimDacVsThresholdForChip->setXaxisTitle("Threshold");
                grTrimDacVsThresholdForChip->setYaxisTitle("TrimDAC");
                Histo1d *hDistThr = new Histo1d("ThresholdDist", 120, 0, 120);
                hDistThr->setXaxisTitle("Threshold [e]");
                hDistThr->setYaxisTitle("Number of channels");

                //Fillint the map and the plots
                for (auto jDvsTrimDac : m_jDvsTrimDac) {
                        unsigned trimDac = jDvsTrimDac.first;
                        for (unsigned iStrip=0; iStrip<128; iStrip++)
                                for (unsigned row=0; row<2; row++) {
                                        std::shared_ptr<const StarJsonData> spJD = jDvsTrimDac.second;
                                        const StarJsonData* jd = spJD.get();
                                        double thr = jd->getValForProp("ABCStar_" + std::to_string(iChip) + "/Threshold/Row" + std::to_string(row), iStrip).value_or(-999);
                                        if (thr!=-999) {
                                                mapThresholdVsTrimDacVsChannelNumber[iStrip + 128*row + iChip*256][trimDac] = thr;
                                                listThresholds.push_back(thr);
                                                grTrimDacVsThresholdForChip->addPoint(thr, trimDac);
                                                hDistThr->fill(thr);
                                        }
                                }
                }
                //Dumping debugging plots
                std::unique_ptr<GraphErrors> upgrTrimDacVsThresholdForChip;
                upgrTrimDacVsThresholdForChip.reset(grTrimDacVsThresholdForChip);
                output->pushData(std::move(upgrTrimDacVsThresholdForChip));
                std::unique_ptr<Histo1d> uphDistThr;
                uphDistThr.reset(hDistThr);
                output->pushData(std::move(uphDistThr));
        }//end of loop over chips to fill in the global map

        //Let's find the threshold 'targetTrh' with the maximum multiplicity of channels
        double targetThr=-1; int maxMultOverAllThresholds=-1;
        sort(listThresholds.begin(), listThresholds.end(), std::greater<double>());
        //for (auto thr : listThresholds) {
        //Will scan the available thresholds with 60 steps (60 is not any magic number, just some historical thing)
        double stepThr = (listThresholds.size()>1) ? (listThresholds[0]-listThresholds[listThresholds.size()-1])/60. : listThresholds[0];
        for (double thr=listThresholds[0]; thr>0.; thr-=stepThr) {
                std::map<unsigned,int> mapOfTrims;
                int mult = getChannelMultReachingTarget(mapThresholdVsTrimDacVsChannelNumber, thr, mapOfTrims);
                alog->debug("For threshold={} got {} channels able to reach it.", thr, mult);
                if (mult > maxMultOverAllThresholds) {
                        maxMultOverAllThresholds = mult;
                        targetThr = thr;
                }
        }
        alog->info("Target threshold is {}", targetThr);

        //Let's retrieve the trims for the target
        std::map<unsigned,int> mapOfBestTrims;
        getChannelMultReachingTarget(mapThresholdVsTrimDacVsChannelNumber, targetThr, mapOfBestTrims);

        //Finding for each channel the value of TrimDac leading to the Threshold the closest to targetThr
        for (unsigned int iChip=0; iChip<(nCol/128); iChip++) {
                //Will dump monitoring plots
                Histo1d *hDistThrBef = new Histo1d("ThresholdDistInit_Chip" + std::to_string(iChip), 120, 0, 120);
                hDistThrBef->setXaxisTitle("Threshold [e]");
                hDistThrBef->setYaxisTitle("Number of channels");
                Histo1d *hDistThrTrimmed = new Histo1d("ThresholdDistTrimmed_Chip" + std::to_string(iChip), 120, 0, 120);
                hDistThrTrimmed->setXaxisTitle("Threshold [e]");
                hDistThrTrimmed->setYaxisTitle("Number of channels");
                //Getting the optimal TrimDAC for each channel
                for (unsigned row=0; row<2; row++) {
                        outJD->initialiseStarChannelsDataAtProp("ABCStar_" + std::to_string(iChip) + "/TrimDAC/Row" + std::to_string(row));
                        for (unsigned iStrip=0; iStrip<128; iStrip++) {
                                unsigned iChannelInTrimMap = iStrip + row*128 + 256*iChip;
                                //if the trim could be computed for this channel we set it to it, otherwise to -1
                                int newTrimDAC=-1;
                                if (mapOfBestTrims.find(iChannelInTrimMap) != mapOfBestTrims.end())
                                        newTrimDAC=mapOfBestTrims[iChannelInTrimMap];
                                outJD->setValForProp("ABCStar_" + std::to_string(iChip) + "/TrimDAC/Row" + std::to_string(row), iStrip, newTrimDAC);
                                //Filling in monitoring plots
                                if (mapThresholdVsTrimDacVsChannelNumber.find(iChannelInTrimMap)!=mapThresholdVsTrimDacVsChannelNumber.end()){
                                        int oldTrimDAC=mapThresholdVsTrimDacVsChannelNumber[iChannelInTrimMap].begin()->first;
                                        hDistThrBef->fill(mapThresholdVsTrimDacVsChannelNumber[iChannelInTrimMap][oldTrimDAC]);
                                        if (newTrimDAC!=-1)
                                                hDistThrTrimmed->fill(mapThresholdVsTrimDacVsChannelNumber[iChannelInTrimMap][newTrimDAC]);
                                        else
                                                hDistThrTrimmed->fill(mapThresholdVsTrimDacVsChannelNumber[iChannelInTrimMap][oldTrimDAC]);
                                }
                        }
                }
                std::unique_ptr<Histo1d> uphDistThrBef;
                uphDistThrBef.reset(hDistThrBef);
                output->pushData(std::move(uphDistThrBef));
                std::unique_ptr<Histo1d> uphDistThrTrimmed;
                uphDistThrTrimmed.reset(hDistThrTrimmed);
                output->pushData(std::move(uphDistThrTrimmed));
        }


        //Dumping full JsonData
        output->pushData(std::move(upJD));
}

void StarTrimDacAnalysis::loadConfig(const json &j) {
    if (j.contains("parametersOfInterest")) {
        for (unsigned i=0; i<j["parametersOfInterest"].size(); i++) {
            m_parametersOfInterest.push_back(j["parametersOfInterest"][i]);
        }
    }
}

unsigned int StarTrimDacAnalysis::getChannelMultReachingTarget(const std::map<unsigned, std::map<int, double> > mapThresholdVsTrimDacVsChannelNumber, double target, std::map<unsigned,int> & mapOfTrims) const {
        unsigned int mult = 0;
        //Looping over channels
        for (auto vals : mapThresholdVsTrimDacVsChannelNumber) {
                unsigned channel = vals.first;
                alog->debug("Trying to reach target {} for channel #{}", target, channel);
                //Looping over TrimDac in decreasing order to find the closest to target
                int bestTrimDacForChannel = -1;
                if (vals.second.size()) {
                        std::map<int, double>::reverse_iterator rit = vals.second.rbegin();
                        double prevThr=rit->second; unsigned prevTrimDac = rit->first;
                        ++rit;
                        while (bestTrimDacForChannel<0 && rit!=vals.second.rend()) {
                                double curThr = rit->second; unsigned curTrimDac = rit->first;
                                //If we target threshold is in between the values for the current and the previous TrimDac we keep the closest
                                if (prevThr >= target && curThr < target) {
                                        if ( (prevThr-target) < (target-curThr) )
                                                bestTrimDacForChannel = prevTrimDac;
                                        else
                                                bestTrimDacForChannel = curTrimDac;
                                }
                                prevTrimDac = curTrimDac;
                                prevThr = target-curThr;
                                ++rit;
                        }
                }
                if (bestTrimDacForChannel>=0) {
                        mapOfTrims[channel] = bestTrimDacForChannel;
                        mult++;
                }
                alog->debug("  returning bestTrimDacForChannel={}", bestTrimDacForChannel);
        }
        return mult;
}

