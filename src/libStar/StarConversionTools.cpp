// #################################
// # Author: Olivier Arnaez & Elise Le Boulicaut
// # Email: Olivier Arnaez at cern.ch
// # Project: Yarr
// # Description: Conversion tools class
// ################################

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

  int i = 0, j=0;

  double factor = 1000.0; // Values stored in V, want them in mV

  FILE* DACtoVConversionFile = fopen(file_name.c_str(),"r");
  if(DACtoVConversionFile == NULL){
    alog->warn("Failed to open threshold calibration file {}", file_name);
  }
  else {
    alog->info("Successfully opened threshold calibration file {}", file_name);
  }
  char line[160 + 1];
  while((fgets(line,160,DACtoVConversionFile)!=NULL)&&(i<256)&&(j<300)){
    int thrDAC;
    float thrConvertedFromFile;
    int narg = sscanf(line,"%d\t%fm",&thrDAC,&thrConvertedFromFile);
    if(narg==2) {    // Expect 2 columns
      m_thrCal_mV[i++]=thrConvertedFromFile * factor;
    }
    j++;
  }
  fclose(DACtoVConversionFile);

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
