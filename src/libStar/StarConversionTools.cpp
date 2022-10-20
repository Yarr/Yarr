// #################################
// # Author: Olivier Arnaez & Elise Le Boulicaut
// # Email: Olivier Arnaez at cern.ch
// # Project: Yarr
// # Description: Conversion tools class
// ################################

#include <fstream>

#include "logging.h"

#include "StarConversionTools.h"

namespace {
    auto alog = logging::make_log("StarConversionTools");
}

StarConversionTools::StarConversionTools() {
  // Default conversion from BVT to mV if no configuration is provided
  // Taken from https://gitlab.cern.ch/atlas-itk-strips-daq/itsdaq-sw/-/blob/master/macros/abc_star/ResponseCurvePlot.cpp#L1343
  m_thrCal_mV.resize(256);
  for(int j=0; j<256; j++) {
    m_thrCal_mV[j] = 2.7264 * j + 1.041;
  }
}

StarConversionTools::StarConversionTools(const std::string& file_name) {
  m_thrCal_mV.resize(256, -1.);

  std::ifstream DACtoVConversionFile(file_name);
  if(!DACtoVConversionFile){
    alog->warn("Failed to open threshold calibration file {}", file_name);
  }
  else {
    alog->info("Successfully opened threshold calibration file {}", file_name);
  }

  // Expect 2 columns in the file
  // The first column is threshold in DAC count; the second is threshold in V
  int thrDAC;
  float thrConvertedFromFile;
  double factor = 1000.0; // convert V to mV

  while (DACtoVConversionFile >> thrDAC >> thrConvertedFromFile) {
    m_thrCal_mV[thrDAC] = thrConvertedFromFile * factor;
  }

  DACtoVConversionFile.close();

  //alog->debug("Got threshold calibration from file:");
  //for(unsigned int i=0; i<256; i++){
  //alog->debug("{}  {}",i,thrConverted[i]);
  //}
}

std::pair<double, double> StarConversionTools::convertDACtoV(double thrDAC, double err_thrDAC){

  int thrBin = (int) thrDAC;
  double thrConverted = -1., err_thrConverted = -1.;
  if((thrBin >= 0) && (thrBin < 256)){
    if(m_thrCal_mV[thrBin] > -1.){
      double remainder = (double)(thrDAC - thrBin);
      thrConverted = m_thrCal_mV[thrBin] + (m_thrCal_mV[thrBin+1] - m_thrCal_mV[thrBin]) * remainder;
      if(thrBin < 128){
	err_thrConverted = err_thrDAC * (m_thrCal_mV[thrBin+1] - m_thrCal_mV[thrBin]);
      } else{
	err_thrConverted = err_thrDAC * (m_thrCal_mV[thrBin] - m_thrCal_mV[thrBin-1]);
      }
    }
  }
  //alog->debug("Converted from DAC to mV: {} +/- {} (DAC) -> {} +/- {} (mV)", thrDAC, err_thrDAC, thrConverted, err_thrConverted);
  return std::make_pair(thrConverted, err_thrConverted);
}

double StarConversionTools::convertBCALtofC(int injDAC) {
  // BCAL: 9 bits for charge range 0 ~ 10 fC
  return injDAC * 10. / (1<<9);
}
