#include "logging.h"

#include "StarConversionTools.h"

namespace {
    auto alog = logging::make_log("StarConversionTools");
}

bool StarConversionTools::loadCalJsonToVec(const json& jcal, std::vector<float>& vec, std::vector<std::pair<int,float>>& vec_pair, unsigned length) {
  vec.clear();
  vec_pair.clear();

  if (jcal.is_array()) {
    // The conversion is provided as an array
    if (jcal.size() != length) {
      alog->error("Array in the config does not have the required length {}", length);
      return false;
    }

    for (unsigned i=0; i<jcal.size(); i++) {
      vec.push_back(jcal[i]);
    }

  } else if (jcal.is_object()) {
    // The conversion is expected to be provided as two arrays: "DAC" and "values"
    if (not jcal.contains("DAC")) {
      alog->error("Cannot load config: feild \"DAC\" is not found");
      return false;
    }
    if (not jcal.contains("values")) {
      alog->error("Cannot load config: field \"values\" is not found");
      return false;
    }

    const auto& DACArr = jcal["DAC"];
    const auto& valArr = jcal["values"];
    if (DACArr.size() != valArr.size()) {
      alog->error("Cannot load config: arrays in \"DAC\" and \"values\" are of different lengths");
      return false;
    }

    // add to the vector of pairs
    for (unsigned i=0; i<DACArr.size(); i++) {
      vec_pair.push_back(std::make_pair(DACArr[i], valArr[i]));
    }

    const unsigned npoints = vec_pair.size();
    if (npoints > length) {
      alog->warn("The provided calibration arrays are longer than what is needed. Extra points are ignored.");
    } else if (npoints < 2) {
      alog->error("Only {} calibration point is provided. Cannot extrapolate!", vec_pair.size());
      return false;
    }

    // Sort based on DAC counts
    std::sort(vec_pair.begin(), vec_pair.end());

    unsigned iCalPoint = 0;
    auto calPoint_low = vec_pair[0];
    auto calPoint_high = vec_pair[1];

    vec.resize(length, -1);
    for (int dac=0; dac<length; dac++) {
      if (dac > calPoint_high.first and iCalPoint+2 < npoints) {
        // Move to the next interval
        iCalPoint++;
        calPoint_low = vec_pair[iCalPoint];
        calPoint_high = vec_pair[iCalPoint+1];
      }

      // Assign the vector based on linear extrapolations of the provided points
      vec[dac] = (calPoint_high.second - calPoint_low.second) / (calPoint_high.first - calPoint_low.first) * (dac - calPoint_low.first) + calPoint_low.second;
    }
  } else if (jcal.is_null()) {
    alog->debug("No calibration is provided. Will use default conversion.");
  }

  return true;
}

void StarConversionTools::loadConfig(const json& j) {
  alog->debug("Load config from json");

  // Trim targets
  if (j.contains("TrimTarget")) {
    auto& targets = j["TrimTarget"];

    // loop over different charge injections for different trim targets
    for (const auto& target : j["TrimTarget"]) {
      unsigned inj = target["BCAL"];
      for (unsigned iabc = 0; iabc < target["BVT"].size(); iabc++) {
        m_trimTargets[inj][iabc] = target["BVT"][iabc];
      }
    }
  }

  // Threshold
  if (j.contains("BVTtoV")) {
    alog->debug("Load \"BVTtoV\"");

    bool loadThr = loadCalJsonToVec(j["BVTtoV"], m_thrCal, m_thrCalPoints, NVALBVT);
    if (not loadThr) {
      alog->error("Failed to load \"BVTtoV\"");
      j["BVTtoV"].dump();
      return;
    }
  }

  // Charge injection
  if (j.contains("BCALtoV")) {
    alog->debug("Load \"BCALtoV\"");
    bool loadInj = loadCalJsonToVec(j["BCALtoV"], m_injCal, m_injCalPoints, NVALBCAL);
    if (not loadInj) {
      alog->error("Failed to load \"BCALtoV\"");
      j["BCALtoV"].dump();
      return;
    }
  }

  // Response curve parameters
  if (j.contains("ResponseFitFunction")) {
    try {
      setResponseFunction(j["ResponseFitFunction"]);
    } catch (const std::exception &e) {
      alog->error("Failed to set response fit function. {}", e.what());
      return;
    }
  }

  if (j.contains("ResponseFitParams")) {
    unsigned ichip = 0;
    for (auto& params : j["ResponseFitParams"]) {
      // check number of parameters
      if (params.size() != m_fitFuncNPars) {
        alog->error("Fucntion \"{}\" requires {} parameters, but {} parameters are provided for chip in input {}.", m_fitFuncName, m_fitFuncNPars, params.size(), ichip);
        continue;
      }

      m_fitParams[ichip];

      for (auto& p : params) {
        m_fitParams[ichip].push_back(p);
      }
      ichip++;
    }
  }
}

void StarConversionTools::writeConfig(json& j) const {
  alog->debug("Write StarConversionTools config to json");

  j["TrimTarget"] = json::array();
  unsigned i_inj = 0;
  for (const auto& [inj, targets] : m_trimTargets) {
    j["TrimTarget"][i_inj]["BCAL"] = inj;

    for (const auto& [iabc, thr] : targets) {
      j["TrimTarget"][i_inj]["BVT"][iabc] = thr;
    }
    i_inj++;
  }

  if (not m_thrCalPoints.empty()) {
    // The original config provides the calibration via two arrays "DAC" and "values"
    j["BVTtoV"]["DAC"] = json::array();
    j["BVTtoV"]["values"] = json::array();

    for (unsigned p = 0; p < m_thrCalPoints.size(); p++) {
      j["BVTtoV"]["DAC"][p] = m_thrCalPoints[p].first;
      j["BVTtoV"]["values"][p] = m_thrCalPoints[p].second;
    }
  } else if (not m_thrCal.empty()) {
    j["BVTtoV"] = json::array();

    for (unsigned i = 0; i < m_thrCal.size(); i++) {
      j["BVTtoV"][i] = m_thrCal[i];
    }
  }

  if (not m_injCalPoints.empty()) {
    // The original config provides the calibration via two arrays "DAC" and "values"
    j["BCALtoV"]["DAC"] = json::array();
    j["BCALtoV"]["values"] = json::array();

    for (unsigned p = 0; p < m_injCalPoints.size(); p++) {
      j["BCALtoV"]["DAC"][p] = m_injCalPoints[p].first;
      j["BCALtoV"]["values"][p] = m_injCalPoints[p].second;
    }
  } else if (not m_injCal.empty()) {
    j["BCALtoV"] = json::array();

    for (unsigned i = 0; i < m_injCal.size(); i++) {
      j["BCALtoV"][i] = m_injCal[i];
    }
  }

  if (not m_fitFuncName.empty()) {
    j["ResponseFitFunction"] = m_fitFuncName;

    j["ResponseFitParams"] = json::array();
    for (const auto& [hccChn, params]: m_fitParams) {
      j["ResponseFitParams"][hccChn] = params;
    }
  }
}

std::pair<float, float> StarConversionTools::convertBVTtomVwithError(float thrDAC, float err_thrDAC) const {

  int thrBin = (int) thrDAC;
  float thrConverted = -1., err_thrConverted = -1.;
  if((thrBin >= 0) && (thrBin < 256)){
    if(convertBVTtomV(thrBin) > -1.){
      float remainder = (float)(thrDAC - thrBin);
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

float StarConversionTools::convertBVTtomV(unsigned thrDAC) const {
  float thrmV = -1;

  if (m_thrCal.empty()) {
    // Default conversion from BVT to mV if no configuration is provided
    // Taken from https://gitlab.cern.ch/atlas-itk-strips-daq/itsdaq-sw/-/blob/master/macros/abc_star/ResponseCurvePlot.cpp#L1343
    thrmV = 2.7264 * thrDAC + 1.041;

  } else if (thrDAC < m_thrCal.size()) {
    // From the calibration config
    thrmV = m_thrCal[thrDAC] * 1000; // convert V to mV
  }

  return thrmV;
}

float StarConversionTools::convertBCALtofC(unsigned injDAC) const {
  float injfC = -1;

  if (m_injCal.empty()) {
    // Default conversion from BCAL to fC if no configuration is provided
    // BCAL: 9 bits for charge range 0 ~ 10 fC
    injfC = injDAC * 10. / (1<<9);
  } else if (injDAC < m_injCal.size()) {
    // Voltage * calibration capacitor
    injfC = m_injCal[injDAC] * injCapfF;
  }

  return injfC;
}

float StarConversionTools::convertfCtomV(float charge, unsigned hccChannel) const {
  // Convert charge in fC to threshold in mV based on the response function
  try {
    auto& params = m_fitParams.at(hccChannel);
    return m_fitFunction(charge, params.data());
  } catch (const std::out_of_range& oor) {
    alog->error("Response curve parameters for ABCStar on HCCStar input channel {} are not available", hccChannel);
    throw;
  }
}

float StarConversionTools::convertmVtofC(float threshold, unsigned hccChannel) const {
  // Convert threshold in mV to charge in fC based on the inverse of the response function
  try {
    auto& params = m_fitParams.at(hccChannel);
    return m_fitFunction_inv(threshold, params.data());
  } catch (const std::out_of_range& oor) {
    alog->error("Response curve parameters for ABCStar on HCCStar input channel {} are not available", hccChannel);
    throw;
  }
}

void StarConversionTools::setResponseFunction(const std::string& functionName) {
  try {
    m_fitFunction = responseFunctions.at(functionName);
    m_fitFunction_inv =  responseFunctions_inv.at(functionName);
    m_fitFuncNPars = funcNParams.at(functionName);
    m_fitFuncName = functionName;
    alog->debug("Set response fit function to {}", functionName);
  } catch (const std::out_of_range& oor) {
    alog->error("Unknown \"ResponseFitFunction\": {}", m_fitFuncName);
    // Let the caller handle the exception
    throw;
  }
}

void StarConversionTools::setResponseParameters(const std::string& functionName, const std::vector<float>& params, unsigned hccChannel) {
  if (functionName != m_fitFuncName) {
    try {
      setResponseFunction(functionName);
    } catch (const std::exception &e) {
      alog->error("Failed to set response fit function and parameters. {}", e.what());
      return;
    }
  }

  // Check if the number of parameters matches the function requirement
  if (params.size() != m_fitFuncNPars) {
    alog->error("Failed to set the function parameters: the required number of parameters is {}, but {} are provided.", m_fitFuncNPars, params.size());
    return;
  }

  m_fitParams[hccChannel] = params;
}

const std::map<std::string, StarConversionTools::RespFuncT> StarConversionTools::responseFunctions {
  {"linear", &StarFitFunction::linFct},
  {"polynomial", &StarFitFunction::polyFct},
  {"exponential", &StarFitFunction::expFct}
};

const std::map<std::string, StarConversionTools::RespFuncT> StarConversionTools::responseFunctions_inv {
  {"linear", &StarFitFunction::linFct_inv},
  {"polynomial", &StarFitFunction::polyFct_inv},
  {"exponential", &StarFitFunction::expFct_inv}
};

const std::map<std::string, unsigned> StarConversionTools::funcNParams {
  {"linear", 2}, {"polynomial", 3}, {"exponential", 3}
};