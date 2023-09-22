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
  unsigned iPOI=0;
  for (const auto& poi : m_parametersOfInterest) {
    for (unsigned n=0; n<s->size(); n++) {
      std::shared_ptr<LoopActionBase> l = s->getLoop(n);
      if (l->isParameterLoop()) {
        auto paramLoop = dynamic_cast<StdParameterLoop*>(l.get());
        if (paramLoop->getParName() == poi) {
          if (iPOI==0)
            parTrimRange_loopindex = n;
          else
            parTrimDac_loopindex = n;
        }
      }
    }
    iPOI++;
  }
}


//! Stores the input StarThresholdResult in the instance for later analysis
/*!
  \param h HistogramBase object that could be a StarThresholdResult storing the partial results of a TrimDac scan
*/
void StarTrimDacAnalysis::processHistogram(HistogramBase *h) {
    std::string hname = h->getName();
    if (h->getName().find("ThresholdMap")!=0) {
        // Looking for threshold data
        return;
    }

    // Get the scan parameter value (TrimDAC)
    unsigned parTrimRange = h->getStat().get(parTrimRange_loopindex);
    unsigned parTrimDac = h->getStat().get(parTrimDac_loopindex);

    // Pull out the relevant threshold information here
    auto trim_key = TrimRangeTrimDac(parTrimRange,parTrimDac);

    fillOneTrimDacInfoFromThreshold(trim_key, *(Histo2d*)h);

    if(!aggregated_loop_status) {
      //Creating a new LoopStatus "agregating" the POI in order to be used in outputs
      std::vector<unsigned> newLSstat;
      std::vector<LoopStyle> newLSstyle;

      LoopStatus lStat = h->getStat();
      for (unsigned n=0; n<lStat.size(); n++) {
        if (n!=parTrimRange_loopindex && n!=parTrimDac_loopindex) {
          newLSstat.push_back( lStat.get(n) );
          newLSstyle.push_back( (LoopStyle) lStat.getStyle(n) );
        }
      }
      aggregated_loop_status = std::make_unique<LoopStatus>(std::move(newLSstat), newLSstyle);
    }
}


//! Initializes an output JsonData object that will store the obtained TrimDAC values
/*!
*/
std::unique_ptr<StarJsonData> StarTrimDacAnalysis::initOutputJsonData() const {
        //Default to empty data
        LoopStatus newLoopStatus;
        if(!aggregated_loop_status) {
            alog->warn("No data arrived to make LoopStatus for StarJsonData object");
        } else {
            newLoopStatus = *aggregated_loop_status;
        }

        alog->debug("creating StarJsonData object to store results");
        std::unique_ptr<StarJsonData> upJD(new StarJsonData("JsonData_StarTrimDACResult", *aggregated_loop_status));
        upJD->setJsonDataType("JsonData_StarTrimDACResult");

        return upJD;
}

//! Fills a map of TrimDac vs Threshold results for each channel identified as iChip * 128 + strip number
void StarTrimDacAnalysis::fillOneTrimDacInfoFromThreshold(TrimRangeTrimDac key, Histo2d &thresh_hist) {
  if(grTrimDacVsThresholdForChip.empty()) {
    // Some debugging plots on the way
    for (unsigned int iChip=0; iChip<(nCol/128); iChip++) {
      grTrimDacVsThresholdForChip.push_back(std::make_unique<GraphErrors>("TrimDacVsThreshold_Chip" + std::to_string(iChip)));
      grTrimDacVsThresholdForChip.back()->setXaxisTitle("Threshold");
      grTrimDacVsThresholdForChip.back()->setYaxisTitle("TrimRange*32 + TrimDAC");

      hDistThr.push_back(std::make_unique<Histo1d>("ThresholdDist", 120, 0, 120));
      hDistThr.back()->setXaxisTitle("Threshold [e]");
      hDistThr.back()->setYaxisTitle("Number of channels");
    }
  }

  //Storing all input results in big map that will later be used to search for the best TrimDac and list of potential targets
  for (unsigned int iChip=0; iChip<(nCol/128); iChip++) {
    //Filling the map and the plots
    for (unsigned iStrip=0; iStrip<128; iStrip++){
      for (unsigned row=0; row<2; row++) {
        // auto thresh_info spJD = jDvsTrimDac.second;

        int bin = thresh_hist.binNum(iChip * 128 + iStrip + 1, row + 1);

        double thr = thresh_hist.getBin(bin);

        mapThresholdVsTrimDacVsChannelNumber[iStrip + 128*row + iChip*256][key] = thr;
        listThresholds.push_back(thr);
        int trimRange = key.range();
        int trimDac = key.dac();

        grTrimDacVsThresholdForChip[iChip]->addPoint(thr, trimRange*32 + trimDac);
        hDistThr[iChip]->fill(thr);
      } //end of loop over rows
    } //end loop over strips
  }//end of loop over chips
}

//! Loops over the inputs and finds the list of used TrimRanges and returns it
/*!
  \param mapThresholdVsTrimDacVsChannelNumber Input map of TrimRange/TrimDac vs Threshold result vs channel
*/
std::vector<int> StarTrimDacAnalysis::getTrimRanges(const std::map<unsigned, std::map<TrimRangeTrimDac, double> > & mapThresholdVsTrimDacVsChannelNumber) const {
  std::vector<int> trimRanges;
  for (auto thVsTRvsTDVsCh : mapThresholdVsTrimDacVsChannelNumber){
    for (auto thVsTRvsTD : thVsTRvsTDVsCh.second) {
      int trimRange = thVsTRvsTD.first.range();
      if (std::find(trimRanges.begin(), trimRanges.end(), trimRange) == trimRanges.end())
        trimRanges.push_back(trimRange);
    }
  }
  return trimRanges;
}

//! Loops over potential target thresholds and retains the ones leading to the maximum channel multiplicity (i.e. maximizing the number of channels able to reach such a target threshold with any value of TrimDac) for each chip or overall
/*!
  \param mapThresholdVsTrimDacVsChannelNumber Input map of TrimRange/TrimDac vs Threshold result vs channel
  \param listThresholds Input list of reached thresholds used to determine the ranger over which to look for a target threshold
  \param outJD Output JsonData object in which are stored the obtained targets
*/
std::map<int, double> StarTrimDacAnalysis::findTargetThresholds(const std::map<unsigned, std::map<TrimRangeTrimDac, double> > & mapThresholdVsTrimDacVsChannelNumber, const std::vector<double> & listThresholds, StarJsonData * outJD) const {
        std::map<int, double> targetThrPerChip;

        //Will collect the list of TrimRanges to scan
        std::vector<int> trimRanges = getTrimRanges(mapThresholdVsTrimDacVsChannelNumber);

        std::map<double,int> multForAllChips;//Will collect info for all chips together in case we want an overall target
        //Probing target thresholds using 60 steps between 0 and the max reachable target (not magic under the '60', just a historical number)
        double minThr = (listThresholds.size()>1) ? listThresholds[listThresholds.size()-1] : 0.;
        double maxThr = listThresholds[0];
        int nStepsThr = 60;
        double stepThr = (listThresholds.size()>1) ? (double)((maxThr-minThr)/nStepsThr) : maxThr;
        for (unsigned int iChip=0; iChip<(nCol/128); iChip++) {
          outJD->initialiseStarChannelsDataAtProp({"ABCStar_" + std::to_string(iChip), "TargetThreshold"}, 1);
          Histo1d *hNTrimmable = new Histo1d("NumTrimmable_Chip" + std::to_string(iChip), nStepsThr+1, minThr-(stepThr/2.), maxThr+(stepThr/2.));
          hNTrimmable->setXaxisTitle("Threshold [e]");
          hNTrimmable->setYaxisTitle("Number of trimmable channels");
          double targetThr=-1; int maxMultOverAllThresholds=-1;
          for (double thr=maxThr; thr>=minThr; thr-=stepThr) {
                std::map<unsigned,int> mapOfTrims;
                int maxMultForChipOverTrimRanges = 0.;
                for (auto trimRange : trimRanges) {
                  int mult = getChannelMultReachingTarget(mapThresholdVsTrimDacVsChannelNumber, trimRange, thr, mapOfTrims, iChip);
                  if (mult > maxMultForChipOverTrimRanges)
                    maxMultForChipOverTrimRanges = mult;
                }
                multForAllChips[thr] += maxMultForChipOverTrimRanges;;
                hNTrimmable->fill(thr, maxMultForChipOverTrimRanges);
                if (maxMultForChipOverTrimRanges > maxMultOverAllThresholds) {
                        maxMultOverAllThresholds = maxMultForChipOverTrimRanges;
                        targetThr = thr;
                }
          }
          alog->info("For ABC {}, target threshold is {}", iChip, targetThr);
          if (m_targetThresholdPerChip){
            outJD->setValForProp({"ABCStar_" + std::to_string(iChip), "TargetThreshold"}, 0, targetThr);
            targetThrPerChip[iChip] = targetThr;
          }
          std::unique_ptr<Histo1d> uphNTrimmable;
          uphNTrimmable.reset(hNTrimmable);
          output->pushData(std::move(uphNTrimmable));
        }
        //Looking for an overall threshold for all chips
        unsigned int maxMultForAllChips=0;
        double targetOverallForAllChips=-1;
        alog->info("Looking for overall target for all chips.");
        for (auto mult = multForAllChips.rbegin(); mult != multForAllChips.rend(); ++mult){
                if (mult->second > maxMultForAllChips) {
                        maxMultForAllChips = mult->second;
                        targetOverallForAllChips = mult->first;
                }
        }

        outJD->initialiseStarChannelsDataAtProp({"OverallTargetThreshold"},1);
        outJD->setValForProp({"OverallTargetThreshold"}, 0, targetOverallForAllChips);
        alog->info("Target threshold overall over all chips would be {}", targetOverallForAllChips);
        //In case we want a common target for all chips, we reset the content of targetThrPerChip (to match the targetOverallForAllChips)
        if (!m_targetThresholdPerChip) {
                alog->info("Setting threshold for all chips to this.");
                for (unsigned int iChip=0; iChip<(nCol/128); iChip++){
                  outJD->setValForProp({"ABCStar_" + std::to_string(iChip), "TargetThreshold"}, 0, targetOverallForAllChips);
                  targetThrPerChip[iChip] = targetOverallForAllChips;
                }
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
void StarTrimDacAnalysis::makeSummaryPlotsForChip(const std::map<unsigned, std::map<TrimRangeTrimDac, double> > & mapThresholdVsTrimDacVsChannelNumber, const int & bestTrimRange, const std::map<unsigned,int> & mapOfBestTrims, StarJsonData * outJD, const int & iChip) const {
                //Will dump monitoring plots
                Histo1d *hDistThrBef = new Histo1d("ThresholdDistInit_Chip" + std::to_string(iChip), 120, 0, 120);
                hDistThrBef->setXaxisTitle("Threshold [e]");
                hDistThrBef->setYaxisTitle("Number of channels");
                Histo1d *hDistThrTrimmed = new Histo1d("ThresholdDistTrimmed_Chip" + std::to_string(iChip), 120, 0, 120);
                hDistThrTrimmed->setXaxisTitle("Threshold [e]");
                hDistThrTrimmed->setYaxisTitle("Number of channels");
                TrimRangeTrimDac defaultTrimDAC(6, 16);
                //Getting the optimal TrimDAC for each channel
                outJD->initialiseStarChannelsDataAtProp({"ABCStar_" + std::to_string(iChip), "TrimRange"}, 1);
                outJD->setValForProp({"ABCStar_" + std::to_string(iChip), "TrimRange"}, 0, bestTrimRange);
                for (unsigned row=0; row<2; row++) {
                        outJD->initialiseStarChannelsDataAtProp({"ABCStar_" + std::to_string(iChip), "TrimDAC", "Row" + std::to_string(row)});
                        for (unsigned iStrip=0; iStrip<128; iStrip++) {
                                unsigned iChannelInTrimMap = iStrip + row*128 + 256*iChip;
                                //if the trim could be computed for this channel we set it to it, otherwise to -1
                                int newTrimDAC=-999;
                                if (mapOfBestTrims.find(iChannelInTrimMap) != mapOfBestTrims.end()) {
                                        newTrimDAC=mapOfBestTrims.at(iChannelInTrimMap);
                                        outJD->setValForProp({"ABCStar_" + std::to_string(iChip), "TrimDAC", "Row" + std::to_string(row)}, iStrip, newTrimDAC);
                                }
                                //Filling in monitoring plots
                                if (mapThresholdVsTrimDacVsChannelNumber.find(iChannelInTrimMap)!=mapThresholdVsTrimDacVsChannelNumber.end()){
                                  if (mapThresholdVsTrimDacVsChannelNumber.at(iChannelInTrimMap).find(defaultTrimDAC)!=mapThresholdVsTrimDacVsChannelNumber.at(iChannelInTrimMap).end()){
                                        TrimRangeTrimDac oldTrimDAC=defaultTrimDAC;
                                        hDistThrBef->fill(mapThresholdVsTrimDacVsChannelNumber.at(iChannelInTrimMap).at(oldTrimDAC));
                                        if (newTrimDAC!=-999){
                                          TrimRangeTrimDac newTrimRangeTrimDac(bestTrimRange,abs(newTrimDAC));;
                                          hDistThrTrimmed->fill(mapThresholdVsTrimDacVsChannelNumber.at(iChannelInTrimMap).at(newTrimRangeTrimDac));
                                        }
                                        else {
                                          hDistThrTrimmed->fill(mapThresholdVsTrimDacVsChannelNumber.at(iChannelInTrimMap).at(oldTrimDAC));
                                        }
                                  }//If even the default TrimDac's results were not available
                                  else {
                                    alog->info("Could not find result for default trim range and trimDAC for channel {}", iChannelInTrimMap);
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
        // Return if no histogram data arrived
        if (listThresholds.empty())
                return;

        //Creating a new LoopStatus "agregating" the POI in order to be used in outputs
        std::unique_ptr<StarJsonData> upJD = std::move(initOutputJsonData());
        StarJsonData * outJD = upJD.get();

        // Send debug plots first
        for (unsigned int iChip=0; iChip<(nCol/128); iChip++) {
          //Dumping debugging plots
          output->pushData(std::move(grTrimDacVsThresholdForChip[iChip]));
          output->pushData(std::move(hDistThr[iChip]));
        }//end of loop over chips to fill in the global map

        //Doing here the search for the best target Threshold looping all m_jDvsTrimDac (i.e. finding the position "threshold_max" of maximum in the distribution of Thresholds having the largest peak)

        //Let's find the threshold 'targetThr' with the maximum multiplicity of channels per chip and overall
        sort(listThresholds.begin(), listThresholds.end(), std::greater<double>()); //we'll start probing the threshold values in decreasing order

        std::map<int, double> targetThrPerChip = findTargetThresholds(mapThresholdVsTrimDacVsChannelNumber, listThresholds, outJD);

        //Let's retrieve the trims for the target, i.e. finding for each channel the value of TrimDac leading to the Threshold the closest to targetThr
        std::map<unsigned, std::map<unsigned,int>> mapOfBestTrims;
        std::map<unsigned,int> bestTrimRangeForChip;
        for (unsigned int iChip=0; iChip<(nCol/128); iChip++) {
          int bestMultForChip=0;
          const std::vector<int> trimRanges = getTrimRanges(mapThresholdVsTrimDacVsChannelNumber);
          for (auto trimRange : trimRanges) {
            std::map<unsigned,int> mapOfTrims;
            unsigned int mult = getChannelMultReachingTarget(mapThresholdVsTrimDacVsChannelNumber, trimRange, targetThrPerChip[iChip], mapOfTrims, iChip);
            if (mult>bestMultForChip) {
              bestMultForChip = mult;
              bestTrimRangeForChip[iChip] = trimRange;
              mapOfBestTrims[iChip] = mapOfTrims;
            }
          }
          makeSummaryPlotsForChip(mapThresholdVsTrimDacVsChannelNumber, bestTrimRangeForChip[iChip], mapOfBestTrims[iChip], outJD, iChip);
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
unsigned int StarTrimDacAnalysis::getChannelMultReachingTarget(const std::map<unsigned, std::map<TrimRangeTrimDac, double> > mapThresholdVsTrimDacVsChannelNumber, const int & trimRange, const double & target, std::map<unsigned,int> & mapOfTrims, const int & iChip) const {
        unsigned int mult = 0;
        //Looping over channels
        for (auto vals : mapThresholdVsTrimDacVsChannelNumber) {
                unsigned channel = vals.first;
                //In case we're interested only in channels from a specific chip (i.e. iChip!=-1), skipping other channels
                if (iChip!=-1 && channel / 256 != iChip)
                  continue;
                //Looping over TrimDac in decreasing order to find the closest to target
                int bestTrimDacForChannel = -999;
                if (vals.second.size()) {
                        std::map<TrimRangeTrimDac, double>::reverse_iterator rit = vals.second.rbegin();

                        //Let's find the first values for the given TrimRange
                        double bestThr=0;
                        while (bestTrimDacForChannel<0 && rit!=vals.second.rend()) {
                          double curThr = rit->second; TrimRangeTrimDac curTrimRangeTrimDac = rit->first;
                          unsigned curTrimRange = curTrimRangeTrimDac.range();  unsigned curTrimDac = curTrimRangeTrimDac.dac();
                          if (curTrimRange == trimRange) {
                            //Taking the first available TrimDAC as best but marked as 'untrimmed' (negative)
                            bestThr=curThr;
                            bestTrimDacForChannel = curTrimDac;
                          }
                          ++rit;
                        }
                        //Let's mark the found bestTrimDac as untrimmed (i.e. negative)
                        bestTrimDacForChannel = -1 * abs(bestTrimDacForChannel);
                        //Now let's find the best trimDac among the following values
                        while (bestTrimDacForChannel<0 && rit!=vals.second.rend()) {
                                double curThr = rit->second; TrimRangeTrimDac curTrimRangeTrimDac = rit->first;
                                unsigned curTrimRange = curTrimRangeTrimDac.range();  unsigned curTrimDac = curTrimRangeTrimDac.dac();
                                if (curTrimRange != trimRange) {
                                  ++rit;
                                  continue;
                                }
                                //If the target threshold is in between the values for the current and the previous best TrimDac we keep the closest as the best TrimDAC and mark the channel as 'trimmed' (positive TrimDac)
                                if (bestThr >= target && curThr < target && std::fabs(bestThr - target) < 5) {
                                  if ( (bestThr-target) < (target-curThr) ) {
                                    bestTrimDacForChannel=abs(bestTrimDacForChannel);
                                  }
                                  else {
                                    bestTrimDacForChannel = curTrimDac;
                                    bestThr = curThr;
                                  }
                                }
                                //Otherwise we keep the TrimDAC leading to the threshold closest to the target but mark it as 'untrimmed' (negative)
                                else {
                                  if ( std::fabs(bestThr-target) < std::fabs(target-curThr) ) {
                                    bestTrimDacForChannel = -1 * abs(bestTrimDacForChannel);
                                  }
                                  else {
                                    bestTrimDacForChannel = -curTrimDac;
                                    bestThr = curThr;
                                  }
                                }
                                ++rit;
                        } //end of second loop over inputs
                }//end of if values exist for channel
                if (bestTrimDacForChannel != -999) {
                        mapOfTrims[channel] = bestTrimDacForChannel;
                        if (bestTrimDacForChannel >= 0){
                          mult++;
                        }
                }
        }
        return mult;
}
