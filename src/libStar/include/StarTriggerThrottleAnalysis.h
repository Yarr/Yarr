#ifndef TRIGTHROTTLEANA_H
#define TRIGTHROTTLEANA_H

// #################################
// // # Author: Olivier Arnaez & Elise Le Boulicaut
// // # Email: Olivier Arnaez at cern.ch
// // # Project: Yarr
// // # Description: Analysis using TriggerThrottleLoop that adjusts the number of triggers per bunch in order to reach a given average occupancy number of hits on the FE
// // # Comment:
// // ################################
//

#include <vector>
//#include <functional>

#include "AnalysisAlgorithm.h"
#include "StdTriggerAction.h"

class HistogramBase;
class Histo2d;


/*! Analysis using TriggerThrottleLoop that adjusts the number of triggers per bunch in order to reach a given average occupancy number of hits on the FE  */
class StarTriggerThrottleAnalysis : public AnalysisAlgorithm {
     public:
 StarTriggerThrottleAnalysis() : AnalysisAlgorithm() {};
    ~StarTriggerThrottleAnalysis() {};
    
    void init(ScanBase *s) override; //!< Initializes the analysis algorithm
    void processHistogram(HistogramBase *h) override; //!< Processes occupancy maps created for each bunch of triggers
    void end() override; //!< Post-scan treatment
    void loadConfig(const json &config) override; //!< Load the configuration of the throttling from json
 private:
    std::vector<unsigned> loops; //!< Keeps track of the scan parameter loops
    std::map<unsigned, std::unique_ptr<Histo2d>> m_occMapOneBunchOfTriggers; //!< Occupancy maps filled for each bunch of triggers, indexed by an identifier of the set of scan parameter values
    std::map<unsigned, std::unique_ptr<Histo2d>> m_occMapAllBunches; //!< Occupancy maps concatenating all bunches of triggers, indexed by an identifier of the set of scan parameter values
    std::map<unsigned, std::unique_ptr<Histo2d>> m_occMapSaturatedChannels; //!< Occupancy maps for channels that are close to saturation (i.e. counter reaching max value -255- in a given bunch of triggers), indexed by an identifier of the set of scan parameter values
    std::map<unsigned, unsigned long>  m_totNbTriggersSoFar; //!< Total number of triggers used so far for each set of scan parameter values

    unsigned m_target_occ=128, m_max_ntriggers=10000; //!< Config parameters of the target (average) occupancy to reach and the maximum number of triggers not to exceed for each set of scan parameter values
    int m_nbTriggersInBunch; //!< Current number of triggers in the bunch

    std::unique_ptr<GlobalFeedbackSender> m_feedback; //!< pointer to the pointer object to send feedback to
    StdTriggerAction* m_trigLoop; //!< pointer to the trigger loop object to modify the number of triggers from
};

#endif
