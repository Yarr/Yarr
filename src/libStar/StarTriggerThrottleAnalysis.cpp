// #################################
// Author: Bruce Gallop
// Based on previous work by Olivier Arnaez, Elise Le Boulicaut and Ryan Roberts

#include "StarTriggerThrottleAnalysis.h"

#include "logging.h"

#include "AllAnalyses.h"
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
    std::shared_ptr<LoopActionBase> l = s->getLoop(n);
    if (!(l->isTriggerLoop() || l->isMaskLoop() || l->isDataLoop() || (l->isParameterLoop() && !isPOILoop(dynamic_cast<StdParameterLoop*>(l.get()))) )){
      loops.push_back(n);
    } else {
      unsigned cnt = (l->getMax() - l->getMin())/l->getStep();
      if (l->isParameterLoop()) {
        cnt++; // Parameter loop interval is inclusive
      }
      if (cnt == 0) {
        cnt = 1;
      }
    }

    if (l->isTriggerLoop()) {
      m_trigLoop = dynamic_cast<StdTriggerAction*>(l.get());
      if(m_trigLoop == nullptr) {
        alog->error("StarTriggerThrottleAnalysis got a trigger loop that can't be cast as it");
      } else {
        //Getting the initial trigger count from the trigger loop parameters
        m_nbTriggersInBunch = m_trigLoop->getTrigCnt();
        alog->info("Starting trigger throttling with a bunch of {} triggers.", m_nbTriggersInBunch);
      }
    }
    //Setting feedback for the trigger throttle
    if (l->isGlobalFeedbackLoop()) {
      m_feedback.reset(new GlobalFeedbackSender(feedback));
      if(m_feedback == nullptr) {
        alog->error("StarTriggerThrottleAnalysis got a GlobalFeedbackLoop that can't be cast as a GlobalFeedbackSender");
      }
    }
  }
}

void StarTriggerThrottleAnalysis::processHistogram(HistogramBase *h) {
    alog->debug("StarTriggerThrottleAnalysis::processHistogram({})", h->getName());

    // Check if right Histogram
    if (h->getName().find(OccupancyMap::outputName()) != 0)
      return;

    // Select correct output container
    unsigned ident = 0;
    unsigned offset = 1;

    // Determine identifier as a function of the scan parameters
    std::string name = "OccupancyMap";
    std::string name2 = "OccupancyMapAllBunches";
    for (unsigned n=0; n<loops.size(); n++) {
        std::shared_ptr<LoopActionBase> l = scan->getLoop(loops[n]);
        ident += ( (h->getStat().get(loops[n])-l->getMin())/l->getStep() )*offset;
        offset *= (l->getMax() - l->getMin())/l->getStep() + 1;
    }
    alog->debug("Continuing with ident {}", ident);

    // Create histogram to store bunch info if not yet made for this bin identifier
    if (m_occMapOneBunchOfTriggers[ident] == NULL) {
      Histo2d *hh = new Histo2d(name, nCol, 0.5, nCol+0.5, nRow, 0.5, nRow+0.5, h->getStat());
      hh->setXaxisTitle("Column");
      hh->setYaxisTitle("Row");
      hh->setZaxisTitle("Hits");
      m_occMapOneBunchOfTriggers[ident].reset(hh);
    }

    // Create "total" histogram (concatenating all bunches of triggers)
    if (m_occMapAllBunches[ident] == NULL) {
      Histo2d *hh = new Histo2d(name2, nCol, 0.5, nCol+0.5, nRow, 0.5, nRow+0.5, h->getStat());
      hh->setXaxisTitle("Column");
      hh->setYaxisTitle("Row");
      hh->setZaxisTitle("Hits");
      m_occMapAllBunches[ident].reset(hh);
      m_totNbTriggersSoFar[ident]=0;
    }

    // Create saturation histogram (occupancies for channels close to saturation)
    if (m_occMapSaturatedChannels[ident] == NULL) {
      Histo2d *hh = new Histo2d("SaturatedChannels", nCol, 0.5, nCol+0.5, nRow, 0.5, nRow+0.5, h->getStat());
      hh->setXaxisTitle("Column");
      hh->setYaxisTitle("Row");
      hh->setZaxisTitle("Hits");
      m_occMapSaturatedChannels[ident].reset(hh);
    }

    // Add up input occupancy map histogram
    m_occMapOneBunchOfTriggers[ident]->add(*(Histo2d*)h);
    m_occMapAllBunches[ident]->add(*(Histo2d*)h);

    // Keep track of how many triggers have been fired so far
    m_totNbTriggersSoFar[ident] += m_nbTriggersInBunch;


    //Determining the sign of change of the number of triggers in the next bunch by looking at the average occupancy in the last bunch of triggers
    short sign = 1;
    alog->debug("About to start loop to determine sign");
    double aveOccupancy = 0.;
    unsigned int nbChannelsAt0=0, nbNoisyChannelsCloseToFullCounter=0, nbNoisyChannelsUsingBestEstimate=0;
    for(unsigned i=0; i<m_occMapOneBunchOfTriggers[ident]->size(); i++) {
      //In case we have channels that were expected to reach saturation and if those are indeed saturated, we use the best estimate of their (relative) occupancy that we kept track of
      if (m_occMapOneBunchOfTriggers[ident]->getBin(i)==255 && m_occMapSaturatedChannels[ident]->getBin(i)) {
        //alog->trace("Using previously computed relative occupancy for channel #{} = {}", i, m_occMapSaturatedChannels[ident]->getBin(i));
        m_occMapOneBunchOfTriggers[ident]->setBin(i, m_occMapSaturatedChannels[ident]->getBin(i)*m_nbTriggersInBunch);
        m_occMapAllBunches[ident]->setBin(i, m_occMapSaturatedChannels[ident]->getBin(i)*m_totNbTriggersSoFar[ident]);
        nbNoisyChannelsUsingBestEstimate++;
      }
      //Let's compute the average absolute occupancy over all channels
      aveOccupancy += m_occMapOneBunchOfTriggers[ident]->getBin(i);
      //printing
      double chOccupancy = (m_occMapOneBunchOfTriggers[ident]->getBin(i))/(double)m_nbTriggersInBunch;
      if (chOccupancy==0)
        nbChannelsAt0++;
      if (m_occMapOneBunchOfTriggers[ident]->getBin(i)>127)
        nbNoisyChannelsCloseToFullCounter++;
    }
    alog->debug("{} channels with 0 occupancy for this bunch of {} triggers", nbChannelsAt0, m_nbTriggersInBunch);
    alog->debug("{} channels with occupancy > 127", nbNoisyChannelsCloseToFullCounter);
    alog->debug("{} channels using previously computed best estimate", nbNoisyChannelsUsingBestEstimate);
    //Computing the average relative (per trigger) occupancy over the whole FE
    aveOccupancy /= (double)(m_occMapOneBunchOfTriggers[ident]->size());
    aveOccupancy /= (double)m_nbTriggersInBunch;
    alog->debug("average relative (i.e. per trigger) occupancy for this bunch of triggers = {}", aveOccupancy);
    //Computing the average obtained absolute efficiency so far
    double totAveOcc = m_occMapAllBunches[ident]->getMean();
    alog->debug("average absolute occupancy so far = {}", totAveOcc);
    //Deciding on whether the nb of triggers in the next bunch is enough to reach the target
    if ((totAveOcc + 0.9*aveOccupancy*m_nbTriggersInBunch/2.)>m_target_occ) { //If we expect to reach the target occupancy with half of the current nb triggers (with a 10% margin) then we divide the nb of triggers in the next bunch by 2
      alog->debug("Setting sign to -1");
      sign = -1;
    } else if ((aveOccupancy*m_nbTriggersInBunch + totAveOcc)<m_target_occ){ //Otherwise, if we expect to be still far from the target we multiply it by 2
      alog->debug("Setting sign to 1");
      sign = 1;
    } else {
      alog->debug("Setting sign to 0"); //Otherwise we stay as we are
      sign = 0;
    }

    //Deciding whether we're done with these scan parameters, either because we reached the target occupancy or we reached the maximum number of triggers
    bool done = m_totNbTriggersSoFar[ident] >= m_max_ntriggers || (totAveOcc > m_target_occ);

    //Setting the number of triggers in the next bunch
    alog->trace("Throttling trigger with {} m_nbTriggersInBunch, sign {}. Total ntrig {}. Max {}.",m_nbTriggersInBunch,sign, m_totNbTriggersSoFar[ident], m_max_ntriggers);
    if (sign == 1) {
      m_nbTriggersInBunch *= 2;
      alog->debug("Setting trigger count to {} for next bunch", m_nbTriggersInBunch);

      //If we're about to double the number of triggers in the next bunch, we keep track of the occupancies for channels that are expected to reach the maximum value allowed by the counter
      for(unsigned i=0; i<m_occMapOneBunchOfTriggers[ident]->size(); i++) {
        if (m_occMapOneBunchOfTriggers[ident]->getBin(i)>127)
          m_occMapSaturatedChannels[ident]->setBin(i, (double)m_occMapAllBunches[ident]->getBin(i)/m_totNbTriggersSoFar[ident]);
      }
    } else if (sign == -1) {
      m_nbTriggersInBunch /= 2;
      alog->debug("Setting trigger count to {} for next bunch", m_nbTriggersInBunch);
    }
    //Let's make sure that we won't exceed the maximum number of triggers allowed
    if (m_nbTriggersInBunch>=m_max_ntriggers)
      m_nbTriggersInBunch = m_max_ntriggers;

    //If we're done with these scan parameters then we compute the relative occupancy map and reinitialize occupancy maps and total number of triggers for the scan parameters
    if (done) {
      alog->debug("Done!");
      m_occMapAllBunches[ident]->scale(1./m_totNbTriggersSoFar[ident]);

      //Dump output plots
      output->pushData(std::move(m_occMapAllBunches[ident]));

      m_occMapAllBunches[ident] = nullptr;
      /*//We need at least as many triggers as the target occupancy for the next set of scan parameters
      if (m_nbTriggersInBunch<m_target_occ)
      m_nbTriggersInBunch = m_target_occ;*/
      m_nbTriggersInBunch=256; //We restart at 256 in order to make sure no channel will be saturated for the first bunch
    } else if (m_nbTriggersInBunch+m_totNbTriggersSoFar[ident] > m_max_ntriggers) { //Otherwise we make sure of not exceeding the max number of triggers allowed
      m_nbTriggersInBunch = m_max_ntriggers - m_totNbTriggersSoFar[ident];
      alog->debug("Setting m_nbTriggersInBunch = {} - {} = {}", m_max_ntriggers, m_totNbTriggersSoFar[ident], m_nbTriggersInBunch);
    }
    //Setting the number of triggers in the next bunch and providing feedback
    m_trigLoop->setTrigCnt(m_nbTriggersInBunch);
    alog->debug("Setting trigger count to {} and calling feedback function", m_nbTriggersInBunch);
    m_feedback->feedback(this->id, sign, done);

    m_occMapOneBunchOfTriggers[ident].reset();
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

  output->pushData(std::move(triggers));
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
    if (j.contains("max_ntriggers")) {
      m_max_ntriggers = (int)j["max_ntriggers"];
    }
}

