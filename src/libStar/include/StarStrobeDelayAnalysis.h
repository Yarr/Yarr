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

class StarStrobeDelayAnalysis : public AnalysisAlgorithm {
    public:
        StarStrobeDelayAnalysis() : AnalysisAlgorithm() {}  //!< Default constructor
        ~StarStrobeDelayAnalysis() override = default;      //!< Default constructor

        void init(const ScanLoopInfo *s) override;        //!< Initializes the analysis ; mostly consists of getting the loop parameter over which data will be aggregated
        void processHistogram(HistogramBase *h) override; //!< Fits the left and right edge of strobe delay pulse for each channel and stores them for further analysis
        void end() override;                              //!< Once all scans inputs have been collected, find highest left edge and lowest right edge to define optimal strobe delay as 57% between the two. Also dump results in StarJsonData file and plots.
        void loadConfig(const json &config) override;     //!< Loads the analysis configuration from a json object

    protected:

        unsigned strobeDelayLoop{};                       //!< Index of loop corresponding to strobe delay parameter

    private:
        unsigned m_strobeDelayMin{};                        //!< Minimum SD
        unsigned m_strobeDelayMax{};                        //!< Maximum SD
        unsigned m_strobeDelayStep{};                       //!< SD step
        unsigned m_strobeDelayBins{};                       //!< Number of SD bins
        unsigned m_injections{};                            //!< Number of injections
        unsigned m_nFailedfit_left{};                    //!< Number of failed left edge fits
	unsigned m_nFailedfit_right{};                   //!< Number of failed right edge fits

        std::vector<double> m_strobeDelayVec;                            //!< Vector of all SD values                

        std::map<unsigned, std::unique_ptr<Histo1d>> m_strobeDelayHistos; //!< Map of histograms of occupancy vs strobe delay. Identifier corresponds to the channel number (0-1279 in first row, 1280-2559 in second row). 
        std::map<unsigned, double> m_leftEdgeMap;            //!< Map of result of right edge fit. Identifier is the same as for histos (above).
	std::map<unsigned, double> m_rightEdgeMap;           //!< Map of result of falling edge fit. Identifier is the same as for histos (above).

        unsigned m_strobeDelayCnt{};                        //!< Counter when incrementing strobe delay

        bool m_dumpDebugSDPlots=false;                    //!< Boolean to dump histograms of occupancy vs strobe delay per channel (every 10 channels for first row only).

        float m_sdFraction = 0.57;                        //!< Fraction to pick between rising and falling edge
	unsigned findBinPassingThreshold(const Histo1d &h_in, float fraction, bool goesAbove, bool goesBelow) const; //!< Function to find first x-axis value for which y-value goes above/below a certain fraction of the maximum
        // Not const decause of m_nFailedfit_left/right
        std::vector<double> fitScurveForSD(const Histo1d &h_in, bool leftEdge, unsigned nBins, double plateauCenter, const std::vector<double> &strobeDelayVec, const std::vector<double> &occVec); //!< Fit a single s-curve (left ot right edge) on a given range of strobe delay pulse
        void splitStrobeDelayRange(const Histo1d &h_in, double & plateauCenter, std::vector<double> & strobeDelayVecLeft, std::vector<double> & strobeDelayVecRight, std::vector<double> & occVecLeft, std::vector<double> & occVecRight) const; //!< Split up strobe delay range into left and right parts to do the two s-curve fits
        // Not const as calls fitScurveForSD 
        std::vector<std::vector<double>> fitDoubleScurve(const Histo1d &h_in); //!< Do the double s-curve fit to obtain rising and falling edge.
	
	std::map<unsigned, std::unique_ptr<Histo2d>> m_hOccVsStrobeDelayVsChannelPerRow; //!< 2D Histogram of occupancy vs strobe delay vs channel number, split per row and per chip. The map identifier is the chip number*2 + row number (both starting at 0)
};


#endif
