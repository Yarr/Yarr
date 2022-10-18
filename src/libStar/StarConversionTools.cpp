// #################################
// # Author: Olivier Arnaez & Elise Le Boulicaut
// # Email: Olivier Arnaez at cern.ch
// # Project: Yarr
// # Description: Conversion tools class
// ################################

#include <cmath>
#include <unistd.h>
#include <vector>
#include "logging.h"

#include "StarConversionTools.h"

namespace {
    auto alog = logging::make_log("StarConversionTools");
}

void StarConversionTools::loadDACtoVConversion(std::vector<double> &thrConverted) {
  thrConverted.resize(256, -1.);

  int i = 0, j=0;

  std::string file_name = "../configs/scans/star/ABCStar_thrCal.txt";
  double factor = 1000.0; // Values stored in V, want them in mV

  FILE* DACtoVConversionFile = fopen(file_name.c_str(),"r");
  if(DACtoVConversionFile == NULL){
    alog->warn("Failed to open threshold calibration file {}", file_name);
    return;
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
      thrConverted[i++]=thrConvertedFromFile * factor;
    }
    j++;
  }
  fclose(DACtoVConversionFile);

  //alog->debug("Got threshold calibration from file:");
  //for(unsigned int i=0; i<256; i++){
  //alog->debug("{}  {}",i,thrConverted[i]);
  //}
}

std::pair<double, double> StarConversionTools::convertDACtoV(const double & thrDAC, const double & err_thrDAC, const std::vector<double> & thrRef_mV){

  int thrBin = (int) thrDAC;
  double thrConverted = -1., err_thrConverted = -1.;
  if((thrBin >= 0) && (thrBin < 256)){
    if(thrRef_mV[thrBin] > -1.){
      double remainder = (double)(thrDAC - thrBin);
      thrConverted = thrRef_mV[thrBin] + (thrRef_mV[thrBin+1] - thrRef_mV[thrBin]) * remainder;
      if(thrBin < 128){
	err_thrConverted = err_thrDAC * (thrRef_mV[thrBin+1] - thrRef_mV[thrBin]);
      } else{
	err_thrConverted = err_thrDAC * (thrRef_mV[thrBin] - thrRef_mV[thrBin-1]);
      }
    }
  }
  //alog->debug("Converted from DAC to mV: {} +/- {} (DAC) -> {} +/- {} (mV)", thrDAC, err_thrDAC, thrConverted, err_thrConverted);
  return std::make_pair(thrConverted, err_thrConverted);
}
