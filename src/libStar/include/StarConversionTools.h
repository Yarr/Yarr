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
#include <cmath>

#include "storage.hpp"

namespace StarFitFunction {
  inline float linFct(float x, const float *par){
    return par[0] + x*par[1];
  }

  inline float polyFct(float x, const float *par){
    return par[0] + x*(par[1]+(par[2]*x));
  }

  inline float expFct(float x, const float *par){
    return par[2] + par[0] / (1 + exp(-x / par[1]));
  }

  // Inverse of the above functions
  inline float linFct_inv(float y, const float *par){
    return (y - par[0]) / par[1];
  }

  inline float polyFct_inv(float y, const float *par){
    // always take the root with the positive sign
    return ( std::sqrt( par[1]*par[1] - 4.0*par[2]*(par[0]-y) ) - par[1] ) / ( 2.0*par[2] );
  }

  inline float expFct_inv(float y, const float *par){
    return -par[1] * std::log( par[0]/(y - par[2]) - 1);
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

  float convertfCtomV(float charge, unsigned hccChannel);
  float convertmVtofC(float threshold, unsigned hccChannel);

  void setResponseFunction(const std::string& functionName);
  std::string getResponseFunctionName() const {return m_fitFuncName;}

  void setResponseParameters(const std::string& functionName, const std::vector<float>& params, unsigned hccChannel);

  std::pair<std::string, std::vector<float>> getResponseParameters(unsigned hccChannel) const {
    return std::make_pair(m_fitFuncName, m_fitParams.at(hccChannel));
  }

  void setTrimTarget(unsigned BCAL, unsigned iABC, unsigned targetBVT) {
    m_trimTargets[BCAL][iABC] = targetBVT;
  }

  void setTrimTarget(unsigned BCAL, const std::map<unsigned, unsigned>& targetBVTs) {
    m_trimTargets[BCAL] = targetBVTs;
  }

  unsigned getTrimTarget(unsigned BCAL, unsigned iABC) const {
    return m_trimTargets.at(BCAL).at(iABC);
  }

  std::map<unsigned, unsigned> getTrimTarget(unsigned BCAL) const {
    return m_trimTargets.at(BCAL);
  }

  using RespFuncT = std::function<float(float, const float *)>;

  static const std::map<std::string, RespFuncT> responseFunctions;
  static const std::map<std::string, RespFuncT> responseFunctions_inv;
  static const std::map<std::string, unsigned> funcNParams;

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
  RespFuncT m_fitFunction;
  RespFuncT m_fitFunction_inv;
  unsigned m_fitFuncNPars;

  // Fit parameters
  std::map<unsigned, std::vector<float>> m_fitParams;

  // Trim targets
  std::map<unsigned, std::map<unsigned, unsigned>> m_trimTargets;
};

#endif