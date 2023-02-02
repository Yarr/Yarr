#ifndef STAR_CONVERSION_TOOL_H
#define STAR_CONVERSION_TOOL_H

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

/// @brief A class to handle the configuration of Star chip calibration parameters
class StarConversionTools {
public:
  StarConversionTools() = default;
  ~StarConversionTools() = default;

  /// Load from json configurations
  void loadConfig(const json& cfg);

  /// Write the current configurations to json
  void writeConfig(json &j) const;

  /// @brief Convert the threshold in DAC counts with error (for example from the S-curve fitting) to threshold in mV.
  /// @param thrDAC Threshold in DAC counts
  /// @param err_thrDAC Error of threshold in DAC counts
  /// @return Threshold in mV, error of threshold in mV
  std::pair<float, float> convertBVTtomVwithError(float thrDAC, float err_thrDAC) const;

  /// @brief Convert the threshold in DAC counts to mV
  /// @param BVT Thresold in DAC counts
  /// @return Threshold in mV
  float convertBVTtomV(unsigned BVT) const;

  /// @brief Convert the charge injection in DAC counts to fC
  /// @param injDAC Charge injection in DAC counts
  /// @return Injected charge in fC
  float convertBCALtofC(unsigned injDAC) const;

  /// @brief Call the response function to convert fC to mV.
  /// @param charge Charge in fC
  /// @param hccChannel ABC chip index for loading the corresponding function parameters
  /// @return Threshold in mV that corresponds to the input charge
  /// @throw std::out_of_range if the function or the parameters are not set
  float convertfCtomV(float charge, unsigned hccChannel) const;

  /// @brief Call the inverse response function to convert mV to fC
  /// @param threshold Threshold in mV
  /// @param hccChannel ABC chip index for loading the corresponding function parameters
  /// @return Charge in fC that corresponds to the threshold
  /// @throw std::out_of_range if the function or the parameters are not set
  float convertmVtofC(float threshold, unsigned hccChannel) const;

  /// @brief Set the response fit function, its inverse function, the number of required parameters, and the function name
  /// @param functionName Name of the function. Available options are: linear, polynomial, exponential
  /// @throw std::out_of_range if the function is unavailable
  void setResponseFunction(const std::string& functionName);

  /// @brief Return the name of the current response function
  std::string getResponseFunctionName() const {return m_fitFuncName;}

  /// @brief Set the response function and its parameters for an ABCStar
  /// @param functionName Name of the function to set
  /// @param params A vector of the function parameters
  /// @param hccChannel ABC chip index
  void setResponseParameters(const std::string& functionName, const std::vector<float>& params, unsigned hccChannel);

  /// @brief Get the response function name and its parameters
  /// @param hccChannel ABC chip index for getting the corresponding parameters
  /// @return The function name and its parameters as a vector
  std::pair<std::string, std::vector<float>> getResponseParameters(unsigned hccChannel) const {
    return std::make_pair(m_fitFuncName, m_fitParams.at(hccChannel));
  }

  /// @brief Set the trim target of an ABC at a given charge injection
  /// @param BCAL Charge injection at which the trim target is set
  /// @param iABC ABC chip index
  /// @param targetBVT Trim target in DAC counts
  void setTrimTarget(unsigned BCAL, unsigned iABC, unsigned targetBVT) {
    m_trimTargets[BCAL][iABC] = targetBVT;
  }

  /// @brief Set the trim target at a given charge injection for all ABCs
  /// @param BCAL Charge injection at which the trim targets are set
  /// @param targetBVTs A map of the trim target for each ABC. The key is the ABC chip index. The value is the trim target in DAC counts.
  void setTrimTarget(unsigned BCAL, const std::map<unsigned, unsigned>& targetBVTs) {
    m_trimTargets[BCAL] = targetBVTs;
  }

  /// @brief Return the trim target of an ABC at a given charge injection
  /// @param BCAL Charge injection at which the trim target is set
  /// @param iABC ABC chip index
  /// @return Trim target in DAC counts
  /// @throw std::out_of_range if BCAL or iABC is not in the map
  unsigned getTrimTarget(unsigned BCAL, unsigned iABC) const {
    return m_trimTargets.at(BCAL).at(iABC);
  }

  /// @brief Return the trim targets of all ABCs at a given charge injection
  /// @param BCAL Charge injection at which the trim target is set
  /// @return A map of the trim target for each ABC. The key is the ABC chip index. The value is the trim target in DAC counts.
  /// @throw std::out_of_range if BCAL is not in the map
  std::map<unsigned, unsigned> getTrimTarget(unsigned BCAL) const {
    return m_trimTargets.at(BCAL);
  }

  using RespFuncT = std::function<float(float, const float *)>;

  /// @brief A map that stores the response fit functions
  static const std::map<std::string, RespFuncT> responseFunctions;

  /// @brief A map that stores the inverse of the response fit function
  static const std::map<std::string, RespFuncT> responseFunctions_inv;

  /// @brief A map that stores the required number of parameters for the response functions
  static const std::map<std::string, unsigned> funcNParams;

private:

  /// A helper function to load calibrations from json configuration
  bool loadCalJsonToVec(const json& jcal, std::vector<float>& vec, std::vector<std::pair<int,float>>& vec_pair, unsigned length);

  /// Number of BVT values for the threshold setting.
  /// This is determined by the number of bits of BVT in AbcCfg.
  static constexpr unsigned NVALBVT = 256;

  /// Number of BCAL values for the charge injection setting.
  /// This is determined by the number of bits of BCAL in AbcCfg.
  static constexpr unsigned NVALBCAL = 512;

  /// Threshold calibration for converting BVT to V.
  /// The index of the vector is BVT DAC counts, the entry is the corresponding threshold in V.
  std::vector<float> m_thrCal;

  /// A vector of {BVT, voltage} pair if the calibration is only provided for a few points
  std::vector<std::pair<int, float>> m_thrCalPoints;

  /// Charge injection calibration for converting BCAL to V.
  /// The index of the vector is BCAL DAC counts, the entry is the corresponding voltage in V for charge injection.
  std::vector<float> m_injCal;

  /// A vector of {BCAL, voltage} pair if the calibration is only provided for a few points
  std::vector<std::pair<int, float>> m_injCalPoints;

  // Capacitance fF of the charge injection capacitor
  float injCapfF {60};

  /// Name of the response curve fit function
  std::string m_fitFuncName;

  /// Response function to convert fC to mV
  RespFuncT m_fitFunction;

  /// Inverse of the response function to convert mV to fC
  RespFuncT m_fitFunction_inv;

  /// Number of the required function parameters
  unsigned m_fitFuncNPars;

  /// A map that stores the response function parameters for each ABCStar.
  /// The key is the HCCStar input channel number following the same indexing convention as StarCfg::m_ABCchips.
  /// The value is the parameters.
  std::map<unsigned, std::vector<float>> m_fitParams;

  /// A nested map that stores the trim targets for ABCStars with various charge injections
  /// The first key is the charge injection value.
  /// The second key is the HCCStar input channel number.
  /// The value in the target BVT for trimming.
  std::map<unsigned, std::map<unsigned, unsigned>> m_trimTargets;
};

#endif