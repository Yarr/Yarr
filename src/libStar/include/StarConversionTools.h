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

#include "storage.hpp"

class StarConversionTools {
public:
  StarConversionTools() = default;
  virtual ~StarConversionTools() = default;

  void loadConfig(const json& cfg);
  void writeConfig(json &j);

  std::pair<double, double> convertDACtomV(double thrDAC, double err_thrDAC);
  double convertBVTtomV(unsigned BVT);
  double convertBCALtofC(unsigned injDAC);

private:

  bool loadCalJsonToVec(const json& cfg, std::vector<double>& vec, unsigned length);

  static constexpr unsigned NVALBVT = 256;
  static constexpr unsigned NVALBCAL = 512;

  // Threshold calibration for converting BVT to V
  std::vector<double> m_thrCal;

  // Charge injection calibration for converting BCAL to V
  std::vector<double> m_injCal;
  double injCapfF {60}; // Capacitance fF of the charge injection capacitor
};

#endif
