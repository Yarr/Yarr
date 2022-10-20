#ifndef STAR_CONVERSION_TOOL_H
#define STAR_CONVERSION_TOOL_H

// #################################
// # Author: Olivier Arnaez & Elise Le Boulicaut
// # Email: Olivier Arnaez at cern.ch
// # Project: Yarr
// # Description: Conversion tools class
// ################################

#include <string>
#include <vector>

class StarConversionTools {
public:
  StarConversionTools() = default;
  virtual ~StarConversionTools() = default;

  void loadConfig(const std::string& file_name);

  std::pair<double, double> convertDACtomV(double thrDAC, double err_thrDAC);
  double convertBVTtomV(unsigned BVT);
  double convertBCALtofC(unsigned injDAC);

private:

  std::vector<double>  m_thrCal_mV;
};

#endif
