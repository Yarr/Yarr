#ifndef TRIGTHROTTLEANA_H
#define TRIGTHROTTLEANA_H

#include <vector>

#include "AnalysisAlgorithm.h"
#include "StdTriggerAction.h"

class HistogramBase;
class Histo2d;

/**
 * Analysis for adjusting trigger according to occupancy.
 *
 * This class implements an analysis algorithm with feedback to the ScanLoop.
 * Based on the occupancy of the histogram, a signal is sent to the
 * StarThrottleLoop.
 *
 * Between the two, the goal is to adjust the number of triggers
 * so that are reasonable number are histogrammed for the current occupancy.
 *
 * The analysis works on a single occupancy histogram from one FrontEnd.
 * Based on repeated values seen in the histogram LoopInfo, it might be
 * possible to combine information from neighbouring histograms.
 *
 * Note that the current version adjusts the trigger from analysis, which
 * is the wrong thing to do.
 */
class StarTriggerThrottleAnalysis : public AnalysisAlgorithm {
public:
    StarTriggerThrottleAnalysis() : AnalysisAlgorithm() {}
    ~StarTriggerThrottleAnalysis() {}

    /**
     * Initialize the analysis algorithm (based on the loop info).
     *
     * @param s scan configuration.
     */
    void init(const ScanLoopInfo *s) override;

    /**
     * Process occupancy maps created for each bunch of triggers.
     *
     * @param h Occupancy map corresponding to the latest bunch of triggers (with scan parameter values)
     */
    void processHistogram(HistogramBase *h) override;

    /// Maybe generate a report after the whole scan
    void end() override;

    /**
     * Load the configuration of the throttling from json.
     *
     * @param j json input configuration.
     */
    void loadConfig(const json &config) override;

private:
    /// Indices of the relevant scan parameter loops
    std::vector<unsigned> loops;

    //!< Occupancy maps concatenating all bunches of triggers, indexed by an identifier of the set of scan parameter values
    std::map<unsigned, std::unique_ptr<Histo2d>> m_occMapAllBunches;
    //!< Occupancy maps for channels that are close to saturation (i.e. counter reaching max value -255- in a given bunch of triggers), indexed by an identifier of the set of scan parameter values
    std::map<unsigned, std::unique_ptr<Histo2d>> m_occMapSaturatedChannels;
    //!< Total number of triggers used so far for each set of scan parameter values
    std::map<unsigned, unsigned long>  m_totNbTriggersSoFar;

    /// The target (average) occupancy to reach
    unsigned m_target_occ=128, m_max_ntriggers=10000;
    /// The maximum number of triggers not to exceed
    int m_nbTriggersInBunch; //!< Current number of triggers in the bunch

    /// Object used to send trigger feedback to ScanLoop
    std::unique_ptr<TriggerFeedbackSender> m_feedback;
    /// The trigger loop object to request trigger count info
    const StdTriggerAction* m_trigLoop;
};

#endif
