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


//! Initializes the analysis ; mostly consists of getting the loop parameter over which data will be aggregated
/*!
*/
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


//! Stores the input StarThresholdResult in the instance for later analysis
/*!
  \param h HistogramBase object that could be a StarThresholdResult storing the partial results of a TrimDac scan
*/
void StarTrimDacAnalysis::processHistogram(HistogramBase *h) {
    std::string hname = h->getName();
    if (h->getName().find("JsonData_StarThresholdResult")!=0)
        return;

    //Getting the input JsonData
    const StarJsonData* jd = new StarJsonData(*((JsonData*)h));
    // Get the scan parameter value (TrimDAC)
    unsigned par = h->getStat().get(par_loopindex);

    m_jDvsTrimDac[par].reset(jd);
}


//! Initializes an output JsonData object that will store the obtained TrimDAC values
/*!
*/
std::unique_ptr<StarJsonData> StarTrimDacAnalysis::initOutputJsonData() const {
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

        return upJD;
}


//! Fills a large map of TrimDac vs Threshold results for each channel identified as iChip * 128 + strip number
/*!
  \param mapThresholdVsTrimDacVsChannelNumber Large output map of TrimDac vs Threshold result vs channel
  \param listThresholds Output list of reached thresholds over all channels (later used to determine the range of threshold over which to look for a target threshold
*/
void StarTrimDacAnalysis::fillGlobalMapOfTrimDacVsThreshold(std::map<unsigned, std::map<int, double> > & mapThresholdVsTrimDacVsChannelNumber, std::vector<double> & listThresholds) const {
        //Storing all input results in big map that will later be used to search for the best TrimDac and list of potential targets
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
                                        alog->info("Before filling map, channel = {}, trimDac = {}, thr = {}", iStrip + 128*row + iChip*255, trimDac, thr);
                                        if (thr!=-999) {
                                          alog->info("Filling map for channel = {}, trimDac = {} with thr = {}", iStrip + 128*row + iChip*255, trimDac, thr);
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
}


//! Loops over potential target thresholds and retains the ones leading to the maximum channel multiplicity (i.e. maximizing the number of channels able to reach such a target threshold with any value of TrimDac) for each chip or overall
/*!
  \param mapThresholdVsTrimDacVsChannelNumber Input map of TrimDac vs Threshold result vs channel
  \param listThresholds Input list of reached thresholds used to determine the ranger over which to look for a target threshold
  \param outJD Output JsonData object in which are stored the obtained targets
*/
std::map<int, double> StarTrimDacAnalysis::findTargetThresholds(const std::map<unsigned, std::map<int, double> > & mapThresholdVsTrimDacVsChannelNumber, const std::vector<double> & listThresholds, StarJsonData * outJD) const {
        std::map<int, double> targetThrPerChip;
        std::map<double,int> multForAllChips;//Will collect info for all chips together in case we want an overall target
        //Probing target thresholds using 60 steps between 0 and the max reachable target (not magic under the '60', just a historical number)
        double stepThr = (listThresholds.size()>1) ? (listThresholds[0]-listThresholds[listThresholds.size()-1])/60. : listThresholds[0];
        for (unsigned int iChip=0; iChip<(nCol/128); iChip++) {
          outJD->initialiseStarChannelsDataAtProp("ABCStar_" + std::to_string(iChip) + "/TargetThreshold", 1);
          Histo1d *hNTrimmable = new Histo1d("NumTrimmable_Chip" + std::to_string(iChip), 60, listThresholds[listThresholds.size()-1], listThresholds[0]);
          hNTrimmable->setXaxisTitle("Threshold [e]");
          hNTrimmable->setYaxisTitle("Number of trimmable channels");
          double targetThr=-1; int maxMultOverAllThresholds=-1;
          for (double thr=listThresholds[0]; thr>0.; thr-=stepThr) {
                std::map<unsigned,int> mapOfTrims;
                int mult = getChannelMultReachingTarget(mapThresholdVsTrimDacVsChannelNumber, thr, mapOfTrims, iChip);
                multForAllChips[thr] += mult;
                alog->debug("For ABC {}, threshold={} got {} channels able to reach it.", iChip, thr, mult);
                hNTrimmable->fill(thr, mult);
                if (mult > maxMultOverAllThresholds) {
                        maxMultOverAllThresholds = mult;
                        targetThr = thr;
                }
          }
          outJD->setValForProp("ABCStar_" + std::to_string(iChip) + "/TargetThreshold", 0, targetThr);
          targetThrPerChip[iChip] = targetThr;
          alog->info("For ABC {}, target threshold is {}", iChip, targetThr);

          std::unique_ptr<Histo1d> uphNTrimmable;
          uphNTrimmable.reset(hNTrimmable);
          output->pushData(std::move(uphNTrimmable));
        }
        //Looking for an overall threshold for all chips
        unsigned int maxMultForAllChips=-1;
        double targetOverallForAllChips=-1;
        for (auto mult : multForAllChips) {
                if (mult.second > maxMultForAllChips) {
                        maxMultForAllChips = mult.second;
                        targetOverallForAllChips = mult.first;
                }
        }

        outJD->initialiseStarChannelsDataAtProp("OverallTrimDacTarget",1);
        outJD->setValForProp("OverallTrimDacTarget", 0, targetOverallForAllChips);
        alog->info("Target threshold overall over all chips would be {}", targetOverallForAllChips);
        //In case we want a common target for all chips, we reset the content of targetThrPerChip (to match the targetOverallForAllChips)
        if (!m_targetThresholdPerChip) {
                alog->info("Setting threshold for all chips to this.");
                for (unsigned int iChip=0; iChip<(nCol/128); iChip++)
                        targetThrPerChip[iChip] = targetOverallForAllChips;
        }

        return targetThrPerChip;
}


//! Dumps optimized TrimDACs in the output JsonData object and some summary plots of obtained thresholds multiplicity before/after TrimDac optimization
/*!
  \param mapThresholdVsTrimDacVsChannelNumber Input map of TrimDac vs Threshold result vs channel
  \param mapOfBestTrims Input map of 'best TrimDAC' (i.e. corresponding to the chosen target) vs channel
  \param outJD Output JsonData object in which are stored the obtained optimised TrimDAC values
  \param iChip Chip index for which to dump the information
*/
void StarTrimDacAnalysis::makeSummaryPlotsForChip(const std::map<unsigned, std::map<int, double> > & mapThresholdVsTrimDacVsChannelNumber, const std::map<unsigned,int> & mapOfBestTrims, StarJsonData * outJD, const int & iChip) const {
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
                                        newTrimDAC=mapOfBestTrims.at(iChannelInTrimMap);
                                outJD->setValForProp("ABCStar_" + std::to_string(iChip) + "/TrimDAC/Row" + std::to_string(row), iStrip, newTrimDAC);
                                //Filling in monitoring plots
                                if (mapThresholdVsTrimDacVsChannelNumber.find(iChannelInTrimMap)!=mapThresholdVsTrimDacVsChannelNumber.end()){
                                        int oldTrimDAC=mapThresholdVsTrimDacVsChannelNumber.at(iChannelInTrimMap).begin()->first;
                                        alog->info("Filling hDistThrBefore for channel {} and TrimDAC {} with Threshold {}", iChannelInTrimMap, oldTrimDAC, mapThresholdVsTrimDacVsChannelNumber.at(iChannelInTrimMap).at(oldTrimDAC));
                                        hDistThrBef->fill(mapThresholdVsTrimDacVsChannelNumber.at(iChannelInTrimMap).at(oldTrimDAC));
                                        if (newTrimDAC!=-1){
                                          if (newTrimDAC >=0){
                                                alog->info("Filling hDistThrTrimmed for channel {} and new positive TrimDAC {} with Threshold {}", iChannelInTrimMap, newTrimDAC, mapThresholdVsTrimDacVsChannelNumber.at(iChannelInTrimMap).at(newTrimDAC));
                                                hDistThrTrimmed->fill(mapThresholdVsTrimDacVsChannelNumber.at(iChannelInTrimMap).at(newTrimDAC));
                                          }
                                          else {
                                                alog->info("Filling hDistThrTrimmed for channel {} and new negative TrimDAC {} with Threshold {}", iChannelInTrimMap, newTrimDAC, mapThresholdVsTrimDacVsChannelNumber.at(iChannelInTrimMap).at(-newTrimDAC));
                                                hDistThrTrimmed->fill(mapThresholdVsTrimDacVsChannelNumber.at(iChannelInTrimMap).at(-newTrimDAC));
                                          }
                                        }
                                        else {
                                          alog->info("Filling hDistThrTrimmed for channel {} and new TrimDAC {} with Threshold {}", iChannelInTrimMap, newTrimDAC, mapThresholdVsTrimDacVsChannelNumber.at(iChannelInTrimMap).at(oldTrimDAC));
                                          hDistThrTrimmed->fill(mapThresholdVsTrimDacVsChannelNumber.at(iChannelInTrimMap).at(oldTrimDAC));
                                        }
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


//! Once all scans inputs have been collected, finds target thresholds, optimises TrimDACs for each channel and dumps the obtained values and control plots
/*!
*/
void StarTrimDacAnalysis::end() {
        if (!m_jDvsTrimDac.size())
                return;

        //Creating a new LoopStatus "agregating" the POI in order to be used in outputs
        std::unique_ptr<StarJsonData> upJD = initOutputJsonData();
        StarJsonData * outJD = upJD.get();

        //Filling in map with all thresholds corresponding to all TrimDac values for all channels
        std::map<unsigned, std::map<int, double> > mapThresholdVsTrimDacVsChannelNumber;
        std::vector<double> listThresholds;
        fillGlobalMapOfTrimDacVsThreshold(mapThresholdVsTrimDacVsChannelNumber, listThresholds);

        //Doing here the search for the best target Threshold looping all m_jDvsTrimDac (i.e. finding the position "threshold_max" of maximum in the distribution of Thresholds having the largest peak)

        //Let's find the threshold 'targetThr' with the maximum multiplicity of channels per chip and overall
        sort(listThresholds.begin(), listThresholds.end(), std::greater<double>()); //we'll start probing the threshold values in decreaseing order
        std::map<int, double> targetThrPerChip = findTargetThresholds(mapThresholdVsTrimDacVsChannelNumber, listThresholds, outJD);

        //Let's retrieve the trims for the target, i.e. finding for each channel the value of TrimDac leading to the Threshold the closest to targetThr
        std::map<unsigned,int> mapOfBestTrims;
        for (unsigned int iChip=0; iChip<(nCol/128); iChip++) {
                getChannelMultReachingTarget(mapThresholdVsTrimDacVsChannelNumber, targetThrPerChip[iChip], mapOfBestTrims, iChip);
                makeSummaryPlotsForChip(mapThresholdVsTrimDacVsChannelNumber, mapOfBestTrims, outJD, iChip);
        }

        //Dumping full JsonData
        output->pushData(std::move(upJD));
}


//! Loads the analysis configuration from a json object
/*!
  \param j Input analysis configuration
*/
void StarTrimDacAnalysis::loadConfig(const json &j) {
    if (j.contains("parametersOfInterest")) {
        for (unsigned i=0; i<j["parametersOfInterest"].size(); i++) {
            m_parametersOfInterest.push_back(j["parametersOfInterest"][i]);
        }
    }

    if (j.contains("targetThresholdPerChip"))
        m_targetThresholdPerChip = j["targetThresholdPerChip"];
}


//! Returns the channel multiplicity (i.e. the number of strips able to reach a given threshold using any value of TrimDAC) for a given threshold 'target' according to the scans input passed in mapThresholdVsTrimDacVsChannelNumber and fills the corresponding TrimDac values in mapOfTrims, doing it for all chips together or only channels of a given chip #iChip
/*!
  \param mapThresholdVsTrimDacVsChannelNumber Input map of TrimDac vs Threshold result vs channel
  \param target Target threshold each channel aims at reaching by optimizing the TrimDAC value
  \param mapOfTrims Output map of TrimDac values for each channel that are best to reach the 'target' threshold
  \param iChip If >=0 the function will return the multiplicity only for the chip #iChip, otherwise will return it for all chips overall (feature not used)
*/
unsigned int StarTrimDacAnalysis::getChannelMultReachingTarget(const std::map<unsigned, std::map<int, double> > mapThresholdVsTrimDacVsChannelNumber, double target, std::map<unsigned,int> & mapOfTrims, int iChip) const {
        unsigned int mult = 0;
        //Looping over channels
        alog->debug("In getChannelMultReachingTarget with chip = {} and target = {}", iChip, target);
        for (auto vals : mapThresholdVsTrimDacVsChannelNumber) {
                unsigned channel = vals.first;
                //In case we're interested only in channels from a specific chip (i.e. iChip!=-1), skipping other channels
                if (iChip!=-1 && channel / 256 != iChip)
                  continue;
                alog->debug("Trying to reach target {} for channel #{}", target, channel);
                //Looping over TrimDac in decreasing order to find the closest to target
                int bestTrimDacForChannel = -1;
                if (vals.second.size()) {
                        std::map<int, double>::reverse_iterator rit = vals.second.rbegin();
                        double prevThr=rit->second; unsigned prevTrimDac = rit->first;
                        ++rit;
                        while (bestTrimDacForChannel<0 && rit!=vals.second.rend()) {
                                double curThr = rit->second; unsigned curTrimDac = rit->first;
                                alog->info("Searching whether channel reaches target. Previous: thr = {}, trim = {} ... New: thr = {}, trim = {}.", prevThr, prevTrimDac, curThr, curTrimDac);
                                //If we target threshold is in between the values for the current and the previous TrimDac we keep the closest
                                if (prevThr >= target && curThr < target) {
                                  if ( (prevThr-target) < (target-curThr) ) {
                                    bestTrimDacForChannel = prevTrimDac;
                                    alog->info("Keeping previous (positive)");
                                  }
                                  else {
                                    bestTrimDacForChannel = curTrimDac;
                                    alog->info("Taking current (positive)");
                                  }
                                }
                                else {
                                  if ( abs(prevThr-target) < abs(target-curThr) ) {
                                    bestTrimDacForChannel = -prevTrimDac;
                                    alog->info("Keeping previous (negative)");
                                  }
                                  else {
                                    bestTrimDacForChannel = -curTrimDac;
                                    alog->info("Taking current (negative)");
                                  }
                                }
                                prevTrimDac = curTrimDac;
                                prevThr = curThr;
                                ++rit;
                        }
                }
                if (bestTrimDacForChannel != -1) {
                        mapOfTrims[channel] = bestTrimDacForChannel;
                        if (bestTrimDacForChannel >= 0){
                          mult++;
                        }
                }
                alog->debug("  returning bestTrimDacForChannel={}", bestTrimDacForChannel);
        }
        return mult;
}
