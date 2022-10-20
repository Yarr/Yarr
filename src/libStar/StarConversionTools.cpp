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

void StarConversionTools::loadConfig(const std::string& file_name) {
  m_thrCal_mV.clear();
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

std::pair<double, double> StarConversionTools::convertDACtomV(double thrDAC, double err_thrDAC){

  int thrBin = (int) thrDAC;
  double thrConverted = -1., err_thrConverted = -1.;
  if((thrBin >= 0) && (thrBin < 256)){
    if(convertBVTtomV(thrBin) > -1.){
      double remainder = (double)(thrDAC - thrBin);
      thrConverted = convertBVTtomV(thrBin) + (convertBVTtomV(thrBin+1) - convertBVTtomV(thrBin)) * remainder;
      if(thrBin < 128){
        err_thrConverted = err_thrDAC * (convertBVTtomV(thrBin+1) - convertBVTtomV(thrBin));
      } else{
        err_thrConverted = err_thrDAC * (convertBVTtomV(thrBin) - convertBVTtomV(thrBin-1));
      }
    }
  }
  //alog->debug("Converted from DAC to mV: {} +/- {} (DAC) -> {} +/- {} (mV)", thrDAC, err_thrDAC, thrConverted, err_thrConverted);
  return std::make_pair(thrConverted, err_thrConverted);
}

double StarConversionTools::convertBVTtomV(unsigned BCAL) {
  double thrmV = -1.;

  if (m_thrCal_mV.empty()) {
    // Default conversion from BVT to mV if no configuration is provided
    // Taken from https://gitlab.cern.ch/atlas-itk-strips-daq/itsdaq-sw/-/blob/master/macros/abc_star/ResponseCurvePlot.cpp#L1343
    thrmV = 2.7264 * BCAL + 1.041;

  } else if (BCAL < m_thrCal_mV.size()) {
    // From the calibration config
    thrmV = m_thrCal_mV[BCAL];
  }

  return thrmV;
}

double StarConversionTools::convertBCALtofC(unsigned injDAC) {
  // BCAL: 9 bits for charge range 0 ~ 10 fC
  return injDAC * 10. / (1<<9);
}
