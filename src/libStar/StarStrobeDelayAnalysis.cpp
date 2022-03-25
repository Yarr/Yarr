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
#include "Histo3d.h"
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
                                []() { return std::unique_ptr<AnalysisAlgorithm>(new StarStrobeDelayFitter());});
}

//! Initializes the analysis ; mostly consists of getting the loop parameter over which data will be aggregated
/*!
*/
void StarStrobeDelayFitter::init(ScanBase *s) {

    scan = s;
    strobeDelayLoop = 0;
    injections = 50;
    for (unsigned n=0; n<s->size(); n++) {
        std::shared_ptr<LoopActionBase> l = s->getLoop(n);

        // Strobe delay Loop
        if (l->isParameterLoop() && isPOILoop(dynamic_cast<StdParameterLoop*>(l.get())) ) {
            strobeDelayLoop = n;
            strobeDelayMax = l->getMax();
            strobeDelayMin = l->getMin();
            strobeDelayStep = l->getStep();
            strobeDelayBins = (strobeDelayMax-strobeDelayMin)/strobeDelayStep;
        }

        if (l->isTriggerLoop()) {
            auto trigLoop = dynamic_cast<StdTriggerAction*>(l.get());
            if(trigLoop == nullptr) {
                alog->error("StarStrobeDelayFitter: loop declared as trigger loop, does not have a trigger count");
            } else {
                injections = trigLoop->getTrigCnt();
            }
        }
    }

    for (unsigned i=strobeDelayMin; i<=strobeDelayMax; i+=strobeDelayStep) {
        x.push_back(i);
    }
    n_failedfit_rising =0;
    n_failedfit_falling =0;
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

//! Does a double s-curve fit to the plot of occupancy vs strobe delay to find the rising and falling edge
/*!
  \param h HistogramBase object that corresponds to an OccupancyMap
*/
void StarStrobeDelayFitter::processHistogram(HistogramBase *h) {

    // Check if right Histogram
    if (h->getName().find(OccupancyMap::outputName()) != 0)
        return;

    Histo2d *hh = (Histo2d*) h;

    std::string strLoopStatus = "";
    for (unsigned i=0; i<hh->getStat().size(); i++)
      strLoopStatus += std::to_string(hh->getStat().get(i)) + " "; 

    strobeDelayCnt++;
    for(unsigned col=1; col<=nCol; col++) {
        for (unsigned row=1; row<=nRow; row++) {
	  unsigned bin = hh->binNum(col, row);
                // Only one loop so ident can simply be obtained from row and column number
	        unsigned ident = ((row-1)*nCol) + (col-1);
                unsigned strobeDelay = hh->getStat().get(strobeDelayLoop);
                std::string name = "StrobeDelay";
                name += "-" + std::to_string(col) + "-" + std::to_string(row);

                // Check if Histogram exists
                if (histos[ident] == NULL) {
                    Histo1d *hOccVsSDPerStrip = new Histo1d(name, strobeDelayBins+1, strobeDelayMin-((double)strobeDelayStep/2.0), strobeDelayMax+((double)strobeDelayStep/2.0));
                    hOccVsSDPerStrip->setXaxisTitle("Strobe Delay");
                    hOccVsSDPerStrip->setYaxisTitle("Occupancy");
                    histos[ident].reset(hOccVsSDPerStrip);
                }

                // Fill per strip histogram
                double thisBin = hh->getBin(bin);
                histos[ident]->fill(strobeDelay, thisBin);

		// Fill 2D histogram of occupancy vs strobe delay vs channel number for each chip and each row
		unsigned iChip = (col-1)/128;
		unsigned iChipRow = iChip + 10*row;
		unsigned binInChip  = (col-1)%128;
		if (hOccVsStrobeDelayVsChannelPerRow[iChipRow] == NULL) {
		  std::string name = "OccVsStrobeDelayVsChanChip" + std::to_string(iChip) + "Row" + std::to_string(row);
		  Histo2d * hOccVsSDVsChPerRow = new Histo2d(name, 128,0,128, 64,0,64);
		  hOccVsSDVsChPerRow->setXaxisTitle("Channel number");
		  hOccVsSDVsChPerRow->setYaxisTitle("Strobe Delay");
		  hOccVsSDVsChPerRow->setZaxisTitle("Occupancy");
		  hOccVsStrobeDelayVsChannelPerRow[iChipRow].reset(hOccVsSDVsChPerRow);
		}
		hOccVsStrobeDelayVsChannelPerRow[iChipRow]->fill(binInChip, strobeDelay, thisBin);


                // Got all data, finish up Analysis
                if (strobeDelayCnt == strobeDelayBins) {
                    // Scale histos
                    //histos[ident]->scale(1.0/(double)injections);
                    const unsigned n_par = 4;
		    //Split up strobe delay range into two parts to do separate s-curve fits
		    unsigned plateauRisingEdge = findBinPassingThreshold(histos[ident].get(), 0.9, true, false);
		    unsigned plateauFallingEdge = findBinPassingThreshold(histos[ident].get(), 0.9, false, true);
		    unsigned plateauCenterBin = (plateauRisingEdge + plateauFallingEdge)/2;
		    double plateauCenter = strobeDelayMin + (strobeDelayStep * plateauCenterBin);
		    std::vector<double> x_rise, x_fall, y_rise, y_fall;
		    x_rise = {x.begin(), x.begin()+plateauCenterBin-1};
		    x_fall = {x.begin()+plateauCenterBin, x.end()};
		    y_rise = {histos[ident]->getData(), histos[ident]->getData() + plateauCenterBin-1};
		    y_fall = {histos[ident]->getData()+plateauCenterBin, histos[ident]->getData()+histos[ident]->size()};
		    unsigned strobeDelayBinsRise = x_rise.size();
		    unsigned strobeDelayBinsFall = x_fall.size();
		    alog->debug(" Results of splitting up strobe delay range: plateauRisingEdge = {}, plateauFallingEdge = {}, plateauCenterBin = {}, plateauCenter = {}", plateauRisingEdge, plateauFallingEdge, plateauCenterBin, plateauCenter);

                    // Rising edge fit
		    lm_status_struct status_rise;
                    lm_control_struct control_rise;
                    control_rise = lm_control_float;
                    control_rise.verbosity = 0;
		    // Guess initial parameters
		    unsigned rise_guess_bin = findBinPassingThreshold(histos[ident].get(), 0.5, true, false);
		    double rise_guess = strobeDelayMin + (strobeDelayStep * rise_guess_bin); 
                    double par_rise[n_par] = {rise_guess,  0.05 * rise_guess , (double) injections, 0};
		    // Do the fit
                    std::chrono::high_resolution_clock::time_point start;
                    std::chrono::high_resolution_clock::time_point end;
                    start = std::chrono::high_resolution_clock::now();
		    lmcurve(n_par, par_rise, strobeDelayBinsRise, &x_rise[0], &y_rise[0], scurveFct, &control_rise, &status_rise);
                    end = std::chrono::high_resolution_clock::now();
                    std::chrono::microseconds fitTime_rise = std::chrono::duration_cast<std::chrono::microseconds>(end-start);
		    double chi2_rise = status_rise.fnorm/(double)(strobeDelayBinsRise - n_par);
		    // Check that the fit results are reasonable
                    if (par_rise[0] > strobeDelayMin && par_rise[0] < plateauCenter && par_rise[1] > 0 && par_rise[1] < (plateauCenter-strobeDelayMin) 
                            && chi2_rise < 2.5 && chi2_rise > 1e-6
                            && fabs((par_rise[2] - par_rise[3])/injections - 1) < 0.1) { 
		      risingEdgeMap[ident] = par_rise[0];
		      alog->debug(" Rising edge fit succeeded for col {} and row {}: p0 = {}, p1 = {}, p2 = {}, p3 = {}", col, row, par_rise[0], par_rise[1], par_rise[2], par_rise[3]);
                    } else {
                        n_failedfit_rising++;
			risingEdgeMap[ident] = -999.;
                        alog->debug("Failed rising edge fit Col({}) Row({}) p0({}) p1 ({}) p2({}) p3 ({}) Chi2({}) Status({}) Entries({})", col, row, par_rise[0], par_rise[1], par_rise[2], par_rise[3], chi2_rise, status_rise.outcome, histos[ident]->getEntries());
                    }

		    // Falling edge fit
		    lm_status_struct status_fall;
                    lm_control_struct control_fall;
                    control_fall = lm_control_float;
                    control_fall.verbosity = 0;
		    // Guess initial parameters
		    unsigned fall_guess_bin = findBinPassingThreshold(histos[ident].get(), 0.5, false, true);
		    double fall_guess = strobeDelayMin + (strobeDelayStep * fall_guess_bin); 
                    double par_fall[n_par] = {fall_guess,  0.05 * fall_guess , (double) injections, 0};
		    // Do the fit
                    start = std::chrono::high_resolution_clock::now();
		    lmcurve(n_par, par_fall, strobeDelayBinsFall, &x_fall[0], &y_fall[0], reverseScurveFct, &control_fall, &status_fall);
                    end = std::chrono::high_resolution_clock::now();
                    std::chrono::microseconds fitTime_fall = std::chrono::duration_cast<std::chrono::microseconds>(end-start);
		    double chi2_fall = status_fall.fnorm/(double)(strobeDelayBinsFall - n_par);
		    // Check that the fit results are reasonable
                    if (par_fall[0] > plateauCenter && par_rise[0] < strobeDelayMax && par_fall[1] > 0 && par_fall[1] < (strobeDelayMax-plateauCenter) 
                            && chi2_fall < 2.5 && chi2_fall > 1e-6
                            && fabs((par_fall[2] - par_fall[3])/injections - 1) < 0.1) { 
		      fallingEdgeMap[ident] = par_fall[0];
		      alog->debug(" Falling edge fit succeeded for col {} and row {}: p0 = {}, p1 = {}, p2 = {}, p3 = {}", col, row, par_fall[0], par_fall[1], par_fall[2], par_fall[3]);
                    } else {
                        n_failedfit_falling++;
		        fallingEdgeMap[ident] = -999.;
                        alog->debug("Failed falling edge fit Col({}) Row({}) p0({}) p1({}) p2 ({}) p3 ({}) Chi2({}) Status({}) Entries({})", col, row, par_fall[0], par_fall[1], par_fall[2], par_fall[3], chi2_fall, status_fall.outcome, histos[ident]->getEntries());
                    }
		    // Dump SD plots if desired
                    if (m_dumpDebugSDPlots && row == nRow/2 && col%10 == 0 || (fallingEdgeMap[ident] ==-999. || risingEdgeMap[ident]==-999.)) {
                        output->pushData(std::move(histos[ident]));
                    }
                    histos[ident].reset(nullptr);
                }
        }
    }
}

//! Once all scan inputs have been collected, finds optimal strobe delay for each chip and dumps results and control plots
/*!
*/
void StarStrobeDelayFitter::end() {

  // Make histograms of rising/falling edge for all channels
  Histo1d *hDistRisingEdge = new Histo1d("RisingEdgeDist", strobeDelayBins, strobeDelayMin, strobeDelayMax);
  hDistRisingEdge->setXaxisTitle("Rising edge");
  hDistRisingEdge->setYaxisTitle("Number of channels");
  Histo1d *hDistFallingEdge = new Histo1d("FallingEdgeDist", strobeDelayBins, strobeDelayMin, strobeDelayMax);
  hDistFallingEdge->setXaxisTitle("Falling edge");
  hDistFallingEdge->setYaxisTitle("Number of channels");
 
  // Create jsondata object to store the results of rising/falling edge for all channels and optimal strobe delay for each chip 
  alog->debug("creating StarJsonData object to store results");
  StarJsonData * outJD = new StarJsonData("JsonData_StarStrobeDelayResult");
  outJD->setJsonDataType("JsonData_StarStrobeDelayResult");
  std::unique_ptr<StarJsonData> upJD;
  upJD.reset(outJD);

  // For each chip, find max rising edge and min falling edge then define optimal strobe delay as the 57% point between the two 
  for (unsigned int iChip=0; iChip<(nCol/128); iChip++){
    outJD->initialiseStarChannelsDataAtProp("ABCStar_" + std::to_string(iChip) + "/OptimalStrobeDelay", 1);
    double maxRisingEdgeForChip = 0.0;
    double minFallingEdgeForChip = 100.0;
    for (unsigned row=0; row<2; row++) {
      outJD->initialiseStarChannelsDataAtProp("ABCStar_" + std::to_string(iChip) + "/RisingEdge/Row" + std::to_string(row));
      outJD->initialiseStarChannelsDataAtProp("ABCStar_" + std::to_string(iChip) + "/FallingEdge/Row" + std::to_string(row));
      for (unsigned iStrip=0; iStrip<128; iStrip++) {
	unsigned iChannel = iStrip + row*nCol + 128*iChip;
	double risingEdgeThisChannel = risingEdgeMap.at(iChannel);
	double fallingEdgeThisChannel = fallingEdgeMap.at(iChannel);
	hDistRisingEdge->fill(risingEdgeThisChannel);
	hDistFallingEdge->fill(fallingEdgeThisChannel);
	outJD->setValForProp("ABCStar_" + std::to_string(iChip) + "/RisingEdge/Row" + std::to_string(row), iStrip, risingEdgeThisChannel);
	outJD->setValForProp("ABCStar_" + std::to_string(iChip) + "/FallingEdge/Row" + std::to_string(row), iStrip, fallingEdgeThisChannel);
	if (risingEdgeThisChannel > maxRisingEdgeForChip){
	  maxRisingEdgeForChip = risingEdgeThisChannel;
	}
	if (fallingEdgeThisChannel < minFallingEdgeForChip && fallingEdgeThisChannel > 0){
	  minFallingEdgeForChip = fallingEdgeThisChannel;
	}
      } // end loop over strips
    } // end loop over rows
    alog->debug("  Found max rising edge = {} and min falling edge = {} for chip {}", maxRisingEdgeForChip, minFallingEdgeForChip, iChip);
    int strobeDelayOpt = -999;
    if (maxRisingEdgeForChip < minFallingEdgeForChip){
      strobeDelayOpt = maxRisingEdgeForChip + 0.57*(minFallingEdgeForChip - maxRisingEdgeForChip);
    }
    alog->debug("  Found optimal strobe delay = {} for chip {}", strobeDelayOpt, iChip);
    outJD->setValForProp("ABCStar_" + std::to_string(iChip) + "/OptimalStrobeDelay", 0, strobeDelayOpt);   
  } // end loop over chips

  double risingEdgeMean = hDistRisingEdge->getMean();
  double risingEdgeStdDev = hDistRisingEdge->getStdDev();
  double fallingEdgeMean = hDistFallingEdge->getMean();
  double fallingEdgeStdDev = hDistFallingEdge->getStdDev();

  // Print out general info
  alog->info("\033[1;33m Rising edge mean = {} +- {}\033[0m", risingEdgeMean, risingEdgeStdDev);
  alog->info("\033[1;33m Falling edge mean = {} +- {}\033[0m", fallingEdgeMean, fallingEdgeStdDev);
  alog->info("\033[1;33m Number of failed fits for rising edge = {}\033[0m", n_failedfit_rising);
  alog->info("\033[1;33m Number of failed fits for falling edge = {}\033[0m", n_failedfit_falling);

  // Output rising/falling edge distributions
  std::unique_ptr<Histo1d> uphDistRisingEdge;
  uphDistRisingEdge.reset(hDistRisingEdge);
  output->pushData(std::move(uphDistRisingEdge));
  std::unique_ptr<Histo1d> uphDistFallingEdge;
  uphDistFallingEdge.reset(hDistFallingEdge);
  output->pushData(std::move(uphDistFallingEdge));
  output->pushData(std::move(upJD));

  // Output occupancy map vs SD vs channel per chip/row
  for (std::map<unsigned, std::unique_ptr<Histo2d>>::iterator i=hOccVsStrobeDelayVsChannelPerRow.begin(); i!=hOccVsStrobeDelayVsChannelPerRow.end(); i++) 
    output->pushData(std::move((*i).second));
   
}


//! Find first x-axis value for which y-value goes above/below a certain fraction of the maximum
/*!
  \param h_in Input histogram
  \param fraction Fraction of the maximum above/below which the y-value should go 
  \param goesAbove Find x-value for which y-value goes above the threshold
  \param goesBelow Find x-value for which y-value goes below the threshold
*/
unsigned StarStrobeDelayFitter::findBinPassingThreshold(const Histo1d *h_in, const float & fraction, const bool & goesAbove, const bool & goesBelow){  
  int bin,i;   
  float y = h_in->getMaximumY()*fraction;   
  if(goesBelow){         // find last bin which is > y          
    for(i=0; i<strobeDelayBins; i++){       
      if(h_in->getBin(i)>y) bin = i;     
    }   
  }else if (goesAbove){             // find first bin which is > y     
    bin = 1;     
    for(i=strobeDelayBins; i>0; i--){       
      if(h_in->getBin(i)>y) bin = i;     
    }   
  }   
  return bin;
} 
