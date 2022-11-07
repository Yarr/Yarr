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

  std::pair<float, float> convertBVTtomVwithError(float thrDAC, float err_thrDAC);
  float convertBVTtomV(unsigned BVT);
  float convertBCALtofC(unsigned injDAC);

  float response(float charge, unsigned hccChannel);

  void setResponseParameters(const std::vector<float>& params, unsigned hccChannel) {
    m_fitParams[hccChannel] = params;
  }

  std::vector<float> getResponseParameters(unsigned hccChannel) {
    return m_fitParams[hccChannel];
  }

private:

  bool loadCalJsonToVec(const json& jcal, std::vector<float>& vec, std::vector<std::pair<int,float>>& vec_pair, unsigned length);

  static constexpr unsigned NVALBVT = 256;
  static constexpr unsigned NVALBCAL = 512;

  // Threshold calibration for converting BVT to V
  std::vector<float> m_thrCal;
  // A vector of {DAC, value} pair if the calibration is only provided for a few points
  std::vector<std::pair<int, float>> m_thrCalPoints;

  // Charge injection calibration for converting BCAL to V
  std::vector<float> m_injCal;
  // A vector of {DAC, value} pair if the calibration is only provided for a few points
  std::vector<std::pair<int, float>> m_injCalPoints;

  // Capacitance fF of the charge injection capacitor
  float injCapfF {60};

  // Response curve fit function
  std::string m_fitFuncName;
  std::function<float(float, const float *)> m_fitFunction;

  // Fit parameters
  std::map<unsigned, std::vector<float>> m_fitParams;
};

#endif