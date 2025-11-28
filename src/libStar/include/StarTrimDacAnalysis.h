#ifndef STAR_TRIMDAC_ANALYSIS_H
#define STAR_TRIMDAC_ANALYSIS_H

// #################################
// # Author: Olivier Arnaez
// # Email: Olivier.Arnaez at cern.ch
// # Project: Yarr
// # Description: Star TrimDAC Analysis class
// ################################

#include <functional>
#include <vector>
#include <iostream>

#include "AnalysisAlgorithm.h"

#include "GraphErrors.h"
#include "Histo2d.h"

class StarJsonData;

struct TrimRangeTrimDac {
  TrimRangeTrimDac(int range, int dac) : _range(range), _dac(dac) {};
  int range() const { return _range;};
  int dac() const { return _dac;};
  bool operator==(const TrimRangeTrimDac &o) const { return _range==o._range && _dac==o._dac; };
  bool operator<(const TrimRangeTrimDac &o)  const { return _range<o._range || (_range==o._range && _dac<o._dac); };
  std::ostream& operator<<(std::ostream& os) {
     os << _range << "/" << _dac << std::endl;
     return os;
  }

  int _range, _dac;
};


class StarTrimDacAnalysis : public AnalysisAlgorithm {
    public:
        StarTrimDacAnalysis() : AnalysisAlgorithm() {} //!< Default constructor
        ~StarTrimDacAnalysis() override = default;     //!< Default destructor

        /**
         * Initializes the analysis.
         *
         * Mostly consists of getting the loop parameter over which data will be aggregated
         */
        void init(const ScanLoopInfo *s) override;
        void processHistogram(HistogramBase *h) override; //!< Stores the input StarThresholdResult in the instance for later analysis
        void end() override; //!< Once all scans inputs have been collected, finds target thresholds, optimises TrimDACs for each channel and dumps the obtained values and control plots
        void loadConfig(const json& config) override;  //!< Loads the analysis configuration from a json object


    private:
	std::map<unsigned, std::map<TrimRangeTrimDac, double> > mapThresholdVsTrimDacVsChannelNumber;
	std::vector<double> listThresholds;

        std::unique_ptr<LoopStatus> aggregated_loop_status;

        std::vector<std::unique_ptr<GraphErrors>> grTrimDacVsThresholdForChip;
        std::vector<std::unique_ptr<Histo1d>> hDistThr;

	unsigned parTrimRange_loopindex=0;                        //!< LoopStatus parameter index of TrimRange during scan
	unsigned parTrimDac_loopindex=0;                          //!< LoopStatus parameter index of TrimDac during scan

	unsigned int getChannelMultReachingTarget(const std::map<unsigned, std::map<TrimRangeTrimDac, double> > & mapThresholdVsTrimDacVsChannelNumber, const unsigned int & trimRange, const double & target, std::map<unsigned,int> & mapOfTrims, const unsigned int & iChip) const; //!< Returns the channel multiplicity (i.e. the number of strips able to reach a given threshold using any value of TrimDAC) for a given threshold 'target' according to the scans input passed in mapThresholdVsTrimDacVsChannelNumber in a given trimDacRange and fills the corresponding TrimDac values in mapOfTrims, doing it for all chips together or only channels of a given chip #iChip

	std::unique_ptr<StarJsonData> initOutputJsonData() const; //!< Initializes an output JsonData object that will store the obtained TrimDAC values

        //! Fill info for one Trim point with Threshold results
        void fillOneTrimDacInfoFromThreshold(TrimRangeTrimDac key, Histo2d &thresh_hist);

	std::map<int, double> findTargetThresholds(const std::map<unsigned, std::map<TrimRangeTrimDac, double> > & mapThresholdVsTrimDacVsChannelNumber, const std::vector<double> & listThresholds, StarJsonData & outJD) const; //!< Loops over potential target thresholds and retains the ones leading to the maximum channel multiplicity (i.e. maximizing the number of channels able to reach such a target threshold with any value of TrimDac) for each chip or overall

	void makeSummaryPlotsForChip(const std::map<unsigned, std::map<TrimRangeTrimDac, double> > & mapThresholdVsTrimDacVsChannelNumber, const int & bestTrimRange, const std::map<unsigned,int> & mapOfBestTrims, StarJsonData & outJD, const int & iChip) const; //!< Dumps optimized TrimDACs in the output JsonData object and some summary plots of obtained thresholds multiplicity before/after TrimDac optimization

	bool m_targetThresholdPerChip = true; //!< Configuration flag to decide whether one computes an overall target threshold for all chips or for each chip individually

	std::vector<int> getTrimRanges(const std::map<unsigned, std::map<TrimRangeTrimDac, double> > & mapThresholdVsTrimDacVsChannelNumber) const; //Returns the list of trim ranges used in inputs

  /// @brief Write trim values to chip configuration
  /// @param chip Chip index
  /// @param range BTRANGE
  /// @param mapOfTrims map of channel index to TrimDAC value
  void writeTrimsToChipConfig(unsigned chip, int range, const std::map<unsigned,int> & mapOfTrims);
};

#endif

