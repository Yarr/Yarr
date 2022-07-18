// #################################
// # Author: Elise Le Boulicaut
// # Email: elise.maria.le.boulicaut@cern.ch
// # Project: Yarr
// # Description: Strobe Delay Analysis class for Star
// ################################

#include "StarStrobeDelayAnalysis.h"
#include "AllAnalyses.h"
#include "StarJsonData.h"
#include "Histo1d.h"
#include "Histo2d.h"
#include "StdHistogrammer.h"
#include "StdTriggerAction.h"
#include "StdParameterLoop.h"
// NB if we don't include this, it compiles, but we get a linker error,
// presumably because it picks up names from C rather than C++
#include <cmath>

#include "lmcurve.h"
#include "logging.h"

namespace {
    auto alog = logging::make_log("StarStrobeDelayAnalysis");
}

namespace {
    bool oa_registered =
      StdDict::registerAnalysis("StarStrobeDelayFitter",
                                []() { return std::make_unique<StarStrobeDelayFitter>();});
}

//! Initializes the analysis ; mostly consists of getting the loop parameter over which data will be aggregated
/*!
*/
void StarStrobeDelayFitter::init(ScanBase *s) {

    scan = s;
    strobeDelayLoop = 0;
    m_injections = 50;
    for (unsigned n=0; n<s->size(); n++) {
        std::shared_ptr<LoopActionBase> l = s->getLoop(n);

        // Strobe delay Loop
        if (l->isParameterLoop() && isPOILoop(dynamic_cast<StdParameterLoop*>(l.get())) ) {
            strobeDelayLoop = n;
            m_strobeDelayMax = l->getMax();
            m_strobeDelayMin = l->getMin();
            m_strobeDelayStep = l->getStep();
            m_strobeDelayBins = (m_strobeDelayMax-m_strobeDelayMin)/m_strobeDelayStep;
        }

        if (l->isTriggerLoop()) {
            auto trigLoop = dynamic_cast<StdTriggerAction*>(l.get());
            if(trigLoop == nullptr) {
                alog->error("StarStrobeDelayFitter: loop declared as trigger loop, does not have a trigger count");
            } else {
                m_injections = trigLoop->getTrigCnt();
            }
        }
    }

    for (unsigned i=m_strobeDelayMin; i<=m_strobeDelayMax; i+=m_strobeDelayStep) {
        m_strobeDelayVec.push_back(i);
    }
    m_nFailedfit_left =0;
    m_nFailedfit_right =0;
}

void StarStrobeDelayFitter::loadConfig(const json &j) {

    if (j.contains("dumpDebugSDPlots")) {
        m_dumpDebugSDPlots = j["dumpDebugSDPlots"];
    }
    if (j.contains("parametersOfInterest")) {
        for (unsigned i=0; i<j["parametersOfInterest"].size(); i++) {
            m_parametersOfInterest.push_back(j["parametersOfInterest"][i]);
        }
    }
}

// Errorfunction
// par[0] = Mean
// par[1] = Sigma
// par[2] = Normlization
// par[3] = Offset
#define SQRT2 1.414213562
double scurveFct(double x, const double *par) {
    return par[3] + 0.5*( 2-erfc( (x-par[0])/(par[1]*SQRT2) ) )*par[2];
}

double reverseScurveFct(double x, const double *par) {
    return par[3] + 0.5*( erfc( (x-par[0])/(par[1]*SQRT2) ) )*par[2];
}

//! Does a double s-curve fit to the plot of occupancy vs strobe delay to find the left and right edge
/*!
  \param h HistogramBase object that corresponds to an OccupancyMap
*/
void StarStrobeDelayFitter::processHistogram(HistogramBase *h) {

    // Check if right Histogram
    if (h->getName().find(OccupancyMap::outputName()) != 0)
        return;

    Histo2d *hh = (Histo2d*) h;

    m_strobeDelayCnt++;
    for(unsigned col=1; col<=nCol; col++) {
        for (unsigned row=1; row<=nRow; row++) {
	  unsigned bin = hh->binNum(col, row);
                // Only one loop so ident can simply be obtained from row and column number
	        unsigned ident = ((row-1)*nCol) + (col-1);
                unsigned strobeDelay = hh->getStat().get(strobeDelayLoop);
                std::string name = "StrobeDelay";
                name += "-" + std::to_string(col) + "-" + std::to_string(row);

                // Check if Histogram exists
                if (m_strobeDelayHistos[ident] == nullptr) {
                    auto hOccVsSDPerStrip = std::make_unique<Histo1d>(name, m_strobeDelayBins+1, m_strobeDelayMin-((double)m_strobeDelayStep/2.0), m_strobeDelayMax+((double)m_strobeDelayStep/2.0));
                    hOccVsSDPerStrip->setXaxisTitle("Strobe Delay");
                    hOccVsSDPerStrip->setYaxisTitle("Occupancy");
                    m_strobeDelayHistos[ident] = std::move(hOccVsSDPerStrip);
                }

                // Fill per strip histogram
                double thisBin = hh->getBin(bin);
                m_strobeDelayHistos[ident]->fill(strobeDelay, thisBin);

		// Fill 2D histogram of occupancy vs strobe delay vs channel number for each chip and each row
		unsigned iChip = (col-1)/128;
		unsigned iChipRow = iChip*2 + (row-1);
		unsigned binInChip  = (col-1)%128;
		if (m_hOccVsStrobeDelayVsChannelPerRow[iChipRow] == nullptr) {
		  std::string name = "OccVsStrobeDelayVsChanChip" + std::to_string(iChip) + "Row" + std::to_string(row);
		  auto hOccVsSDVsChPerRow = std::make_unique<Histo2d>(name, 128, 0, 128, m_strobeDelayBins+1, m_strobeDelayMin-((double)m_strobeDelayStep/2.0), m_strobeDelayMax+((double)m_strobeDelayStep/2.0));
		  hOccVsSDVsChPerRow->setXaxisTitle("Channel number");
		  hOccVsSDVsChPerRow->setYaxisTitle("Strobe Delay");
		  hOccVsSDVsChPerRow->setZaxisTitle("Occupancy");
		  m_hOccVsStrobeDelayVsChannelPerRow[iChipRow] = std::move(hOccVsSDVsChPerRow);
		}
		m_hOccVsStrobeDelayVsChannelPerRow[iChipRow]->fill(binInChip, strobeDelay, thisBin);


                // Got all data, finish up Analysis
                if (m_strobeDelayCnt == m_strobeDelayBins) {

		  // Do double s-curve fit
		  std::vector<std::vector<double>> fitParams = fitDoubleScurve(m_strobeDelayHistos[ident].get());
		  m_leftEdgeMap[ident] = fitParams[0][0];
		  m_rightEdgeMap[ident] = fitParams[1][0];

		  // Dump SD plots if desired
		  if (m_dumpDebugSDPlots && row == nRow/2 && col%10 == 0 || (m_rightEdgeMap[ident] ==-999. || m_leftEdgeMap[ident]==-999.)) {
		    output->pushData(std::move(m_strobeDelayHistos[ident]));
		  }
		  m_strobeDelayHistos[ident].reset(nullptr);
                }
        }
    }
}

//! Once all scan inputs have been collected, finds optimal strobe delay for each chip and dumps results and control plots
/*!
*/
void StarStrobeDelayFitter::end() {

  // Make histograms of left/right edge for all channels
  auto hDistLeftEdge = std::make_unique<Histo1d>("LeftEdgeDist", m_strobeDelayBins+1, m_strobeDelayMin-((double)m_strobeDelayStep/2.0), m_strobeDelayMax+((double)m_strobeDelayStep/2.0));
  hDistLeftEdge->setXaxisTitle("Left edge");
  hDistLeftEdge->setYaxisTitle("Number of channels");
  auto hDistRightEdge = std::make_unique<Histo1d>("RightEdgeDist", m_strobeDelayBins+1, m_strobeDelayMin-((double)m_strobeDelayStep/2.0), m_strobeDelayMax+((double)m_strobeDelayStep/2.0));
  hDistRightEdge->setXaxisTitle("Right edge");
  hDistRightEdge->setYaxisTitle("Number of channels");
 
  // Create jsondata object to store the results of left/right edge for all channels and optimal strobe delay for each chip 
  alog->debug("creating StarJsonData object to store results");
  auto upJD = std::make_unique<StarJsonData>("JsonData_StarStrobeDelayResult");
  upJD->setJsonDataType("JsonData_StarStrobeDelayResult");

  // For each chip, find max left edge and min right edge then define optimal strobe delay as the 57% point between the two 
  for (unsigned int iChip=0; iChip<(nCol/128); iChip++){
    upJD->initialiseStarChannelsDataAtProp("ABCStar_" + std::to_string(iChip) + "/OptimalStrobeDelay", 1);
    double maxLeftEdgeForChip = 0.0;
    double minRightEdgeForChip = 100.0;
    for (unsigned row=0; row<2; row++) {
      upJD->initialiseStarChannelsDataAtProp("ABCStar_" + std::to_string(iChip) + "/LeftEdge/Row" + std::to_string(row));
      upJD->initialiseStarChannelsDataAtProp("ABCStar_" + std::to_string(iChip) + "/RightEdge/Row" + std::to_string(row));
      for (unsigned iStrip=0; iStrip<128; iStrip++) {
	unsigned iChannel = iStrip + row*nCol + 128*iChip;
	double leftEdgeThisChannel = m_leftEdgeMap.at(iChannel);
	double rightEdgeThisChannel = m_rightEdgeMap.at(iChannel);
	hDistLeftEdge->fill(leftEdgeThisChannel);
	hDistRightEdge->fill(rightEdgeThisChannel);
	upJD->setValForProp("ABCStar_" + std::to_string(iChip) + "/LeftEdge/Row" + std::to_string(row), iStrip, leftEdgeThisChannel);
	upJD->setValForProp("ABCStar_" + std::to_string(iChip) + "/RightEdge/Row" + std::to_string(row), iStrip, rightEdgeThisChannel);
	if (leftEdgeThisChannel > maxLeftEdgeForChip){
	  maxLeftEdgeForChip = leftEdgeThisChannel;
	}
	if (rightEdgeThisChannel < minRightEdgeForChip && rightEdgeThisChannel > 0){
	  minRightEdgeForChip = rightEdgeThisChannel;
	}
      } // end loop over strips
    } // end loop over rows
    alog->debug("  Found max left edge = {} and min right edge = {} for chip {}", maxLeftEdgeForChip, minRightEdgeForChip, iChip);
    int strobeDelayOpt = -999;
    if (maxLeftEdgeForChip < minRightEdgeForChip){
      strobeDelayOpt = maxLeftEdgeForChip + 0.57*(minRightEdgeForChip - maxLeftEdgeForChip);
    }
    alog->debug("  Found optimal strobe delay = {} for chip {}", strobeDelayOpt, iChip);
    upJD->setValForProp("ABCStar_" + std::to_string(iChip) + "/OptimalStrobeDelay", 0, strobeDelayOpt);   
  } // end loop over chips

  double leftEdgeMean = hDistLeftEdge->getMean();
  double leftEdgeStdDev = hDistLeftEdge->getStdDev();
  double rightEdgeMean = hDistRightEdge->getMean();
  double rightEdgeStdDev = hDistRightEdge->getStdDev();

  // Print out general info
  alog->info("\033[1;33m Left edge mean = {} +- {}\033[0m", leftEdgeMean, leftEdgeStdDev);
  alog->info("\033[1;33m Right edge mean = {} +- {}\033[0m", rightEdgeMean, rightEdgeStdDev);
  alog->info("\033[1;33m Number of failed fits for left edge = {}\033[0m", m_nFailedfit_left);
  alog->info("\033[1;33m Number of failed fits for right edge = {}\033[0m", m_nFailedfit_right);

  // Output left/right edge distributions
  output->pushData(std::move(hDistLeftEdge));
  output->pushData(std::move(hDistRightEdge));
  output->pushData(std::move(upJD));

  // Output occupancy map vs SD vs channel per chip/row
  for (unsigned int row=0; row<2; row++){
    auto hOccVsSDVsCh = std::make_unique<Histo2d>("OccVsStrobeDelayVsChan_Row" + std::to_string(row), 1280, 0, 1280, m_strobeDelayBins+1,  m_strobeDelayMin-((double)m_strobeDelayStep/2.0), m_strobeDelayMax+((double)m_strobeDelayStep/2.0));
    for (unsigned int iChip=0; iChip<(nCol/128); iChip++) {
      Histo1d * hOccVsStrobeDelayPerRow = m_hOccVsStrobeDelayVsChannelPerRow[iChip*2 + row]->profileY();
      const unsigned n_par = 4;
      upJD->initialiseStarChannelsDataAtProp("ABCStar_" + std::to_string(iChip) + "/FitParamsLeft/Row" + std::to_string(row), n_par);
      upJD->initialiseStarChannelsDataAtProp("ABCStar_" + std::to_string(iChip) + "/FitParamsRight/Row" + std::to_string(row), n_par);
      upJD->initialiseStarChannelsDataAtProp("ABCStar_" + std::to_string(iChip) + "/Width/Row" + std::to_string(row), 1);
      for (unsigned int iStrip=0; iStrip<128; iStrip++){
	for (unsigned int sd=m_strobeDelayMin; sd<=m_strobeDelayMax; sd+=m_strobeDelayStep){
	  int binNum = m_hOccVsStrobeDelayVsChannelPerRow[iChip*2 + row]->binNum(iStrip, sd);
	  double binContent = m_hOccVsStrobeDelayVsChannelPerRow[iChip*2 + row]->getBin(binNum);
	  hOccVsSDVsCh->fill(iChip*128 + iStrip, sd, binContent);
	} // end loop over points
      } // end loop over strips
      std::vector<std::vector<double>> fitParamsPerRow = fitDoubleScurve(hOccVsStrobeDelayPerRow);
      for (unsigned int ipar=0; ipar<n_par; ipar++){
	upJD->setValForProp("ABCStar_" + std::to_string(iChip) + "/FitParamsLeft/Row" + std::to_string(row), ipar, fitParamsPerRow[0][ipar]);
	upJD->setValForProp("ABCStar_" + std::to_string(iChip) + "/FitParamsRight/Row" + std::to_string(row), ipar, fitParamsPerRow[1][ipar]);
      }
      upJD->setValForProp("ABCStar_" + std::to_string(iChip) + "/Width/Row" + std::to_string(row), 0, fitParamsPerRow[1][0] - fitParamsPerRow[0][0]);
      std::unique_ptr<Histo1d> uphOccVsStrobeDelayPerRow;
      uphOccVsStrobeDelayPerRow.reset(hOccVsStrobeDelayPerRow);
      output->pushData(std::move(uphOccVsStrobeDelayPerRow));
    } // end loop over chips
    hOccVsSDVsCh->setXaxisTitle("Channel number");
    hOccVsSDVsCh->setYaxisTitle("Strobe Delay");
    hOccVsSDVsCh->setZaxisTitle("Occupancy");
    output->pushData(std::move(hOccVsSDVsCh));
  }
  for (std::map<unsigned, std::unique_ptr<Histo2d>>::iterator i=m_hOccVsStrobeDelayVsChannelPerRow.begin(); i!=m_hOccVsStrobeDelayVsChannelPerRow.end(); i++) {
    output->pushData(std::move((*i).second));
  }
   
}


//! Find first x-axis value for which y-value goes above/below a certain fraction of the maximum
/*!
  \param h_in Input histogram
  \param fraction Fraction of the maximum above/below which the y-value should go 
  \param goesAbove Find x-value for which y-value goes above the threshold
  \param goesBelow Find x-value for which y-value goes below the threshold
*/
unsigned StarStrobeDelayFitter::findBinPassingThreshold(const Histo1d *h_in, float fraction, bool goesAbove, bool goesBelow){  
  int bin,i;   
  float y = h_in->getMaximumY()*fraction;   
  if(goesBelow){         // find last bin which is > y          
    for(i=0; i<m_strobeDelayBins; i++){       
      if(h_in->getBin(i)>y) bin = i;     
    }   
  }else if (goesAbove){             // find first bin which is > y     
    bin = 1;     
    for(i=m_strobeDelayBins; i>0; i--){       
      if(h_in->getBin(i)>y) bin = i;     
    }   
  }   
  return bin;
}

//! Fit a single s-curve (left ot right edge) on a given range of strobe delay pulse. 
/*!
  \param h_in Input histogram (full strobe delay pulse)
  \param leftEdge Boolean to decide whether to fit the left edge (leftEdge = true) or right edge (leftEdge = false)
  \param n_par Number of parameters in the fit 
  \param nBins Number of bins in the desied range
  \param plateauCenter Point of separation between the left and right ranges
  \param strobeDelayVec Strobe delay values in the desired range
  \param occVec Occupancy values in the desired range 
*/
std::vector<double> StarStrobeDelayFitter::fitScurveForSD(const Histo1d *h_in, bool leftEdge, unsigned n_par, unsigned nBins, double plateauCenter, const std::vector<double> &strobeDelayVec, const std::vector<double> &occVec){
  lm_status_struct status;
  lm_control_struct control;
  control = lm_control_float;
  control.verbosity = 0;
  // Guess initial parameters
  unsigned p0initial_bin; double p0initial; double par[n_par] = {};
  if (leftEdge){
    p0initial_bin = findBinPassingThreshold(h_in, 0.5, true, false);
    p0initial = m_strobeDelayMin + (m_strobeDelayStep * p0initial_bin); 
    
  } else {
    p0initial_bin = findBinPassingThreshold(h_in, 0.5, false, true);
    p0initial = m_strobeDelayMin + (m_strobeDelayStep * p0initial_bin); 
  }
  par[0] = p0initial; par[1] = 0.05 * p0initial; par[2] = (double)m_injections; par[3] = 0.;
  // Do the fit
  if (leftEdge)
    lmcurve(n_par, par, nBins, &strobeDelayVec[0], &occVec[0], scurveFct, &control, &status);
  else
    lmcurve(n_par, par, nBins, &strobeDelayVec[0], &occVec[0], reverseScurveFct, &control, &status);
  double chi2 = status.fnorm/(double)(nBins - n_par);
  // Check that the fit results are reasonable
  std::vector<double> fitParams;
  double minRange, maxRange;
  if (leftEdge){
    minRange = m_strobeDelayMin;
    maxRange = plateauCenter;
  } else {
    minRange = plateauCenter;
    maxRange = m_strobeDelayMax;
  }
  if (par[0] > minRange && par[0] < maxRange && par[1] > 0 && par[1] < (maxRange-minRange) 
      && chi2 < 10
      && fabs((par[2] - par[3])/m_injections - 1) < 0.1) {
    alog->debug("Fit succeeded for {} with leftEdge {}: p0 = {}, p1 = {}, p2 = {}, p3 = {}", h_in->getName(), leftEdge, par[0], par[1], par[2], par[3]);
    for (unsigned int ipar=0; ipar<n_par; ipar++)
      fitParams.push_back(par[ipar]);
  } else {
    if (leftEdge)
      m_nFailedfit_left++;
    else
      m_nFailedfit_right++;
    alog->debug("Fit failed for {} with leftEdge {}: p0 = {}, p1 = {}, p2 = {}, p3 = {}", h_in->getName(), leftEdge, par[0], par[1], par[2], par[3]);
    if (par[0] < minRange || par[0] > maxRange)
      alog->debug("p0 out of range");
    else if ( par[1] < 0 || par[1] > (maxRange-minRange))
      alog->debug("p1 out of range");
    else if (chi2 > 10)
      alog->debug("chi2 = {} > 10", chi2);
    else if ( fabs((par[2] - par[3])/m_injections - 1) > 0.1)
    alog->debug("(p2 - p3)/m_injections - 1 = {} > 0.1", fabs((par[2] - par[3])/m_injections - 1));
    for (unsigned int ipar=0; ipar<n_par; ipar++)
      fitParams.push_back(-999.);
  }
  return fitParams;
}

//! Split up strobe delay range into left and right parts to do the two s-curve fits. Returns point of separation between the two ranges and vectors of strobe delay and occupancy for the left and right parts.
/*!
  \param h_in Input histogram (full strobe delay pulse)
  \param plateauCenter Point of separation between the left and right ranges
  \param strobeDelayVec Strobe delay values in the desired range
  \param occVec Occupancy values in the desired range 
*/
void StarStrobeDelayFitter::splitStrobeDelayRange(const Histo1d *h_in, double & plateauCenter, std::vector<double> & strobeDelayVecLeft, std::vector<double> & strobeDelayVecRight, std::vector<double> & occVecLeft, std::vector<double> & occVecRight){

  //Split up strobe delay range into two parts to do separate s-curve fits
  unsigned plateauLeftEdge = findBinPassingThreshold(h_in, 0.9, true, false);
  unsigned plateauRightEdge = findBinPassingThreshold(h_in, 0.9, false, true);
  unsigned plateauCenterBin = (plateauLeftEdge + plateauRightEdge)/2;
  plateauCenter = m_strobeDelayMin + (m_strobeDelayStep * plateauCenterBin);
  alog->debug(" Results of splitting up strobe delay range for {}: plateauLeftEdge = {}, plateauRightEdge = {}, plateauCenterBin = {}, plateauCenter = {}", h_in->getName(), plateauLeftEdge, plateauRightEdge, plateauCenterBin, plateauCenter);
  strobeDelayVecLeft = {m_strobeDelayVec.begin(), m_strobeDelayVec.begin() + plateauCenterBin-1};
  strobeDelayVecRight = {m_strobeDelayVec.begin() + plateauCenterBin, m_strobeDelayVec.end()};
  occVecLeft = {h_in->getData(), h_in->getData() + plateauCenterBin-1};
  occVecRight = {h_in->getData() + plateauCenterBin, h_in->getData() + h_in->size()};
}

//! Do the double s-curve fit to obtain rising and falling edge. Returns vector of vectors containing fit parameters for left side (index 0) and right side (index 1).
/*!
  \param h_in Input histogram (full strobe delay pulse)
 */
std::vector<std::vector<double>> StarStrobeDelayFitter::fitDoubleScurve(const Histo1d *h_in){
  //Split up strobe delay range into two parts to do separate s-curve fits
  double plateauCenter; std::vector<double> strobeDelayVec_left, strobeDelayVec_right, occVec_left, occVec_right;
  splitStrobeDelayRange(h_in, plateauCenter, strobeDelayVec_left, strobeDelayVec_right, occVec_left, occVec_right);
  unsigned nStrobeDelayBinsLeft = strobeDelayVec_left.size();
  unsigned nStrobeDelayBinsRight = strobeDelayVec_right.size();
		  
  // Do the fits
  const unsigned n_par = 4;
  std::vector<std::vector<double>> fitParams;
  
  // Left edge fit
  fitParams.push_back( fitScurveForSD(h_in, true, n_par, nStrobeDelayBinsLeft, plateauCenter, strobeDelayVec_left, occVec_left) );

  // Right edge fit
  fitParams.push_back( fitScurveForSD(h_in, false, n_par, nStrobeDelayBinsRight, plateauCenter, strobeDelayVec_right, occVec_right) );

  return fitParams;

}
 
