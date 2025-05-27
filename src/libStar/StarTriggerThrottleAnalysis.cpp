// #################################
// Author: Bruce Gallop
// Based on previous work by Olivier Arnaez, Elise Le Boulicaut and Ryan Roberts

#include "StarTriggerThrottleAnalysis.h"

#include "logging.h"

#include "AllAnalyses.h"
#include "Histo1d.h"
#include "Histo2d.h"
#include "StdHistogrammer.h"
#include "StdTriggerAction.h"
#include "StdParameterLoop.h"

//Defining logger name
namespace {
    auto alog = logging::make_log("StarTriggerThrottleAnalysis");
}

//Registering the analysis algorithm in the registry
namespace {
    bool throt_registered =
        StdDict::registerAnalysis("StarTriggerThrottleAnalysis", []() { return std::unique_ptr<AnalysisAlgorithm>(new StarTriggerThrottleAnalysis());});
}

void StarTriggerThrottleAnalysis::init(const ScanLoopInfo *s) {
  //Getting the scan parameters to keep track of loop statuses/identify outputs
  for (unsigned n=0; n<s->size(); n++) {
    auto l = s->getLoop(n);
    if (!(l->isTriggerLoop() || l->isMaskLoop() || l->isDataLoop() || (l->isParameterLoop() && !isPOILoop(l)))) {
      // Parameter loops and feedback
      ident_loops.push_back(n);
      loopMax.push_back((unsigned)l->getMax());
    }

    if (l->isTriggerLoop()) {
      m_trigLoop = dynamic_cast<const StdTriggerAction*>(l);
      if(m_trigLoop == nullptr) {
        alog->error("StarTriggerThrottleAnalysis got a trigger loop that can't be cast as it");
      } else {
        // Logging the initial trigger count from the trigger loop parameters
        auto startTriggers = m_trigLoop->getTrigCnt();
        alog->info("Starting trigger throttling with a bunch of {} triggers.", startTriggers);
      }
    }
    //Setting feedback for the trigger throttle
    if (l->isTriggerFeedbackLoop()) {
      m_feedback.reset(new TriggerFeedbackSender(feedback));
      if(m_feedback == nullptr) {
        alog->error("StarTriggerThrottleAnalysis got a TriggerFeedbackLoop that can't be cast as a TriggerFeedbackSender");
      }
    }
  }
}

unsigned StarTriggerThrottleAnalysis::buildIdent(const LoopStatus &ls) {
    // Select correct output container
    unsigned ident = 0;
    unsigned offset = 1;

    // Determine identifier as a function of the scan parameters
    for (unsigned n=0; n<ident_loops.size(); n++) {
        auto loop_val = ls.get(ident_loops[n]);
        ident += loop_val*offset;
        offset *= loopMax[n];
    }

    return ident;
}

void StarTriggerThrottleAnalysis::processHistogram(HistogramBase *h) {
    alog->debug("StarTriggerThrottleAnalysis::processHistogram({})", h->getName());

    // Only processing occupancy histograms
    if (h->getName().find(OccupancyMap::outputName()) != 0) {
      return;
    }

    // Number of triggers for this histogram
    unsigned nbTriggersInBunch = m_trigLoop->getTrigCnt();

    // Select correct output container
    unsigned ident = buildIdent(h->getStat());

    // Determine identifier as a function of the scan parameters
    std::string name2 = "OccupancyMapAllBunches";

    alog->debug("Histogram from {} triggers for ident {}.",
                nbTriggersInBunch, ident);

    // Create "total" histogram (concatenating all bunches of triggers)
    if (m_occMapAllBunches[ident] == nullptr) {
      Histo2d *hh = new Histo2d(name2, nCol, 0.5, nCol+0.5, nRow, 0.5, nRow+0.5, h->getStat());
      hh->setXaxisTitle("Column");
      hh->setYaxisTitle("Row");
      hh->setZaxisTitle("Hits");
      m_occMapAllBunches[ident].reset(hh);
      m_totNbTriggersSoFar[ident]=0;
    }

    // Create saturation histogram (occupancies for channels close to saturation)
    if (m_occMapSaturatedChannels[ident] == nullptr) {
      Histo2d *hh = new Histo2d("SaturatedChannels", nCol, 0.5, nCol+0.5, nRow, 0.5, nRow+0.5, h->getStat());
      hh->setXaxisTitle("Column");
      hh->setYaxisTitle("Row");
      hh->setZaxisTitle("Hits");
      m_occMapSaturatedChannels[ident].reset(hh);
    }

    auto curr_occ = dynamic_cast<const Histo2d *>(h);

    // Add up input occupancy map histogram
    m_occMapAllBunches[ident]->add(*curr_occ);

    // Keep track of how many triggers have been fired so far
    m_totNbTriggersSoFar[ident] += nbTriggersInBunch;


    //Determining the sign of change of the number of triggers in the next bunch by looking at the average occupancy in the last bunch of triggers
    alog->debug("About to start loop to determine sign");
    double aveOccupancy = 0.;
    unsigned int nbChannelsAt0=0, nbNoisyChannelsCloseToFullCounter=0, nbNoisyChannelsUsingBestEstimate=0;
    for(unsigned i=0; i<curr_occ->size(); i++) {
      //In case we have channels that were expected to reach saturation and if those are indeed saturated, we use the best estimate of their (relative) occupancy that we kept track of
      auto curr_val = curr_occ->getBin(i);
      auto saturation_occupancy = m_occMapSaturatedChannels[ident]->getBin(i);
      if (curr_val==255 && saturation_occupancy) {
        //alog->trace("Using previously computed relative occupancy for channel #{} = {}", i, saturation_occupancy);
        curr_val = saturation_occupancy*nbTriggersInBunch;
        m_occMapAllBunches[ident]->setBin(i, saturation_occupancy*m_totNbTriggersSoFar[ident]);
        nbNoisyChannelsUsingBestEstimate++;
      }
      //Let's compute the average absolute occupancy over all channels
      aveOccupancy += curr_val;
      //printing
      double chOccupancy = curr_val/(double)nbTriggersInBunch;
      if (chOccupancy==0)
        nbChannelsAt0++;
      if (curr_val>127)
        nbNoisyChannelsCloseToFullCounter++;
    }
    alog->debug("{} channels with 0 occupancy for this bunch of {} triggers", nbChannelsAt0, nbTriggersInBunch);
    alog->debug("{} channels with occupancy > 127", nbNoisyChannelsCloseToFullCounter);
    alog->debug("{} channels using previously computed best estimate", nbNoisyChannelsUsingBestEstimate);
    //Computing the average relative (per trigger) occupancy over the whole FE
    aveOccupancy /= (double)(curr_occ->size());
    aveOccupancy /= (double)nbTriggersInBunch;
    alog->debug("average relative (i.e. per trigger) occupancy for this bunch of triggers = {}", aveOccupancy);
    //Computing the average obtained absolute efficiency so far
    double totAveOcc = m_occMapAllBunches[ident]->getMean();
    alog->debug("average absolute occupancy so far = {}", totAveOcc);
    short sign = 1;
    //Deciding on whether the nb of triggers in the next bunch is enough to reach the target
    if ((totAveOcc + 0.9*aveOccupancy*nbTriggersInBunch/2.)>m_target_occ) { //If we expect to reach the target occupancy with half of the current nb triggers (with a 10% margin) then we divide the nb of triggers in the next bunch by 2
      alog->debug("Setting sign to -1 {} + {} > {}",
                  totAveOcc, 0.9*aveOccupancy*nbTriggersInBunch/2.,
                  m_target_occ);
      sign = -1;
    } else if ((aveOccupancy*nbTriggersInBunch + totAveOcc)<m_target_occ){ //Otherwise, if we expect to be still far from the target we multiply it by 2
      alog->debug("Setting sign to 1 {} {} < {}",
                  aveOccupancy*nbTriggersInBunch, totAveOcc, m_target_occ);
      sign = 1;
    } else {
      alog->debug("Setting sign to 0"); //Otherwise we stay as we are
      sign = 0;
    }

    //Deciding whether we're done with these scan parameters, because we reached the target occupancy
    bool done = totAveOcc > m_target_occ;

    //Setting the number of triggers in the next bunch
    alog->trace("Throttling trigger with {} nbTriggersInBunch, sign {}. Total ntrig {}.",nbTriggersInBunch,sign, m_totNbTriggersSoFar[ident]);
    if (sign == 1) {
      //If we're about to double the number of triggers in the next bunch, we keep track of the occupancies for channels that are expected to reach the maximum value allowed by the counter
      for(unsigned i=0; i<curr_occ->size(); i++) {
        if (curr_occ->getBin(i)>127)
          m_occMapSaturatedChannels[ident]->setBin(i, (double)m_occMapAllBunches[ident]->getBin(i)/m_totNbTriggersSoFar[ident]);
      }
    }

    //If we're done with these scan parameters then we compute the relative occupancy map and reinitialize occupancy maps and total number of triggers for the scan parameters
    if (done) {
      alog->debug("Done!");
      m_occMapAllBunches[ident]->scale(1./m_totNbTriggersSoFar[ident]);

      //Dump output plots
      output->pushData(std::move(m_occMapAllBunches[ident]));

      m_occMapAllBunches[ident] = nullptr;
      /*//We need at least as many triggers as the target occupancy for the next set of scan parameters
      if (nbTriggersInBunch<m_target_occ)
      nbTriggersInBunch = m_target_occ;*/
      nbTriggersInBunch=256; //We restart at 256 in order to make sure no channel will be saturated for the first bunch
    }

    alog->debug("Calling feedback function {} {}", this->id, sign);
    m_feedback->feedbackTrigger(this->id, sign);
}

void StarTriggerThrottleAnalysis::end() {
  // Store num triggers for each bin in separate histogram
  auto len = m_totNbTriggersSoFar.size();
  auto trigRecord = std::make_unique<Histo1d>
    ("NumTriggers", len, -0.5, len + 0.5);

  size_t index = 0;
  // List in order by ident
  for(auto &items: m_totNbTriggersSoFar) {
    auto count = items.second;
    trigRecord->setBin(index++, count);
  }

  output->pushData(std::move(trigRecord));
}

void StarTriggerThrottleAnalysis::loadConfig(const json &j) {
    alog->warn("In loadConfig()");
    if (j.contains("parametersOfInterest")) {
        for (unsigned i=0; i<j["parametersOfInterest"].size(); i++) {
            m_parametersOfInterest.push_back(j["parametersOfInterest"][i]);
        }
    }
    if (j.contains("target_occ")) {
      m_target_occ = (int)j["target_occ"];
      alog->info("having m_target_occ={}", (int)m_target_occ);
    }
}

