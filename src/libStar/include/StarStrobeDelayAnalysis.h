#ifndef STAR_STROBE_DELAY_ANALYSIS_H
#define STAR_STROBE_DELAY_ANALYSIS_H

// #################################
// # Author: Elise Le Boulicaut
// # Email: elise.maria.le.boulicaut@cern.ch
// # Project: Yarr
// # Description: Strobe Delay Analysis class for Star
// ################################

#include <vector>
#include <functional>

#include "AnalysisAlgorithm.h"

class HistogramBase;
class Histo1d;
class Histo2d;
class Histo3d;

class StarStrobeDelayFitter : public AnalysisAlgorithm {
    public:
        StarStrobeDelayFitter() : AnalysisAlgorithm() {}  //!< Default constructor
        ~StarStrobeDelayFitter() override = default;      //!< Default constructor

        void init(ScanBase *s) override;                  //!< Initializes the analysis ; mostly consists of getting the loop parameter over which data will be aggregated
        void processHistogram(HistogramBase *h) override; //!< Fits the rising and falling edge for each channel and stores them for further analysis
        void end() override;                              //!< Once all scans inputs have been collected, find max rising edge and min falling edge to define optimal strobe delay as 57% between the two. also dump results in StarJsonData file and plots.
        void loadConfig(const json &config) override;     //!< Loads the analysis configuration from a json object

    protected:

        unsigned strobeDelayLoop{};                       //!< Index of loop corresponding to strobe delay parameter

    private:
        unsigned strobeDelayMin{};                        //!< Minimum SD
        unsigned strobeDelayMax{};                        //!< Maximum SD
        unsigned strobeDelayStep{};                       //!< SD step
        unsigned strobeDelayBins{};                       //!< Number of SD bins
        unsigned injections{};                            //!< Number of injections
        unsigned n_failedfit_rising{};                    //!< Number of failed rising edge fits
	unsigned n_failedfit_falling{};                   //!< Number of failed falling edge fits

        std::vector<double> x;                            //!< Vector of all SD values                

        std::map<unsigned, std::unique_ptr<Histo1d>> histos; //!< Map of histograms of occupancy vs strobe delay. Identifier corresponds to the channel number (0-1279 in first row, 1280-2559 in second row). 
        std::map<unsigned, double> risingEdgeMap;            //!< Map of result of rising edge fit. Identifier is the same as for histos (above).
	std::map<unsigned, double> fallingEdgeMap;           //!< Map of result of falling edge fit. Identifier is the same as for histos (above).

        unsigned strobeDelayCnt{};                        //!< Counter when incrementing strobe delay

        bool m_dumpDebugSDPlots=false;                    //!< Boolean to dump histograms of occupancy vs strobe delay per channel (every 10 channels for first row only).

	unsigned findBinPassingThreshold(const Histo1d *h_in, const float & fraction, const bool & goesAbove, const bool & goesBelow); //!< Function to find first x-axis value for which y-value goes above/below a certain fraction of the maximum
	
	std::map<unsigned, std::unique_ptr<Histo2d>> hOccVsStrobeDelayVsChannelPerRow; //!< 2D Histogram of occupancy vs strobe delay vs channel number, split per row and per chip. The map identifier is the chip number + 10*row number
};


#endif
