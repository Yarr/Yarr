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
#include <functional>

#include "storage.hpp"

namespace FitFunction {
  inline float linFct(float x, const float *par){
    return par[0] + x*par[1];
  }

  inline float polyFct(float x, const float *par){
    return par[0] + x*(par[1]+(par[2]*x));
  }

  inline float expFct(float x, const float *par){
    return par[2] + par[0] / (1 + exp(-x / par[1]));
  }
}

class StarConversionTools {
public:
  StarConversionTools() = default;
  virtual ~StarConversionTools() = default;

  void loadConfig(const json& cfg);
  void writeConfig(json &j);

  std::pair<float, float> convertDACtomV(float thrDAC, float err_thrDAC);
  float convertBVTtomV(unsigned BVT);
  float convertBCALtofC(unsigned injDAC);

private:

  bool loadCalJsonToVec(const json& cfg, std::vector<float>& vec, unsigned length);

  static constexpr unsigned NVALBVT = 256;
  static constexpr unsigned NVALBCAL = 512;

  // Threshold calibration for converting BVT to V
  std::vector<float> m_thrCal;

  // Charge injection calibration for converting BCAL to V
  std::vector<float> m_injCal;
  float injCapfF {60}; // Capacitance fF of the charge injection capacitor
};

#endif