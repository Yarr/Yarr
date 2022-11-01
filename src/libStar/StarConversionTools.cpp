#include "logging.h"

#include "StarConversionTools.h"

namespace {
    auto alog = logging::make_log("StarConversionTools");
}

bool StarConversionTools::loadCalJsonToVec(const json& jcal, std::vector<float>& vec, unsigned length) {
  vec.clear();

  if (jcal.is_value_array()) {
    // The conversion is provided as an array
    vec = jcal;
    if (vec.size() != length) {
      alog->error("Array in the config does not have the required length {}", length);
      return false;
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

    auto& DACArr = jcal["DAC"];
    auto& valArr = jcal["values"];
    if (DACArr.size() != valArr.size()) {
      alog->error("Cannot load config: arrays in \"DAC\" and \"values\" are of different lengths");
      return false;
    }

    // make a vector of pairs
    std::vector<std::pair<int, float>> DACValArr;
    for (unsigned i=0; i<DACArr.size(); i++) {
      DACValArr.push_back(std::make_pair(DACArr[i], valArr[i]));
    }

    unsigned npoints = DACValArr.size();
    if (npoints > length) {
      alog->warn("The provided calibration arrays are longer than what is needed. Extra points are ignored.");
    } else if (npoints < 2) {
      alog->error("Only {} calibration point is provided. Cannot extrapolate!", DACValArr.size());
      return false;
    }

    // Sort based on DAC counts
    std::sort(DACValArr.begin(), DACValArr.end());

    unsigned iCalPoint = 0;
    auto calPoint_low = DACValArr[0];
    auto calPoint_high = DACValArr[1];

    vec.resize(length, -1);
    for (int dac=0; dac<length; dac++) {
      if (dac > calPoint_high.first and iCalPoint+2 < npoints) {
        // Move to the next interval
        iCalPoint++;
        calPoint_low = DACValArr[iCalPoint];
        calPoint_high = DACValArr[iCalPoint+1];
      }

      // Assign the vector based on linear extrapolations of the provided points
      vec[dac] = (calPoint_high.second - calPoint_low.second) / (calPoint_high.first - calPoint_low.first) * (dac - calPoint_low.first) + calPoint_low.second;
    }
  }

  return true;
}

void StarConversionTools::loadConfig(const json& j) {
  alog->debug("Load config from json");

  // Threshold
  if (j.contains("BVTtoV")) {
    alog->debug("Load \"BVTtoV\"");

    bool loadThr = loadCalJsonToVec(j["BVTtoV"], m_thrCal, NVALBVT);
    if (not loadThr) {
      alog->error("Failed to load \"BVTtoV\"");
      j["BVTtoV"].dump();
      return;
    }
  }

  // Charge injection
  if (j.contains("BCALtoV")) {
    alog->debug("Load \"BCALtoV\"");
    bool loadInj = loadCalJsonToVec(j["BCALtoV"], m_injCal, NVALBCAL);
    if (not loadInj) {
      alog->error("Failed to load \"BCALtoV\"");
      j["BCALtoV"].dump();
      return;
    }
  }

  // Response curve parameters
  if (j.contains("ResponseFitFunction")) {
    m_fitFuncName = j["ResponseFitFunction"];
    if (m_fitFuncName == "linear") {
      m_fitFunction = &FitFunction::linFct;
    } else if (m_fitFuncName == "polynomial") {
      m_fitFunction = &FitFunction::polyFct;
    } else if (m_fitFuncName == "exponential") {
      m_fitFunction = &FitFunction::expFct;
    } else {
      alog->error("Unknown \"ResponseFitFunction\": {}", m_fitFuncName);
      return;
    }
  }

  if (j.contains("ResponseFitParams")) {
    unsigned ichip = 0;
    for (auto& params : j["ResponseFitParams"]) {
      m_fitParams[ichip];
      // m_fitParams[ichip] = params; gives an "Unexpected index" exception
      for (auto& p : params) {
        m_fitParams[ichip].push_back(p);
      }
      ichip++;
    }
  }
}

void StarConversionTools::writeConfig(json& j) {
  alog->debug("Write StarConversionTools config to json");

  j["BVTtoV"] = m_thrCal;

  j["BCALtoV"] = m_injCal;

  j["ResponseFitFunction"] = m_fitFuncName;

  j["ResponseFitParams"] = json::array();
  for (const auto& [hccChn, params]: m_fitParams) {
    j["ResponseFitParams"][hccChn] = params;
  }
}

std::pair<float, float> StarConversionTools::convertDACtomV(float thrDAC, float err_thrDAC){

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

float StarConversionTools::convertBVTtomV(unsigned thrDAC) {
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

float StarConversionTools::convertBCALtofC(unsigned injDAC) {
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

float StarConversionTools::response(float charge, unsigned hccChannel) {
  // Convert threshold to charge
  std::vector<float> params;

  try {
    params = m_fitParams.at(hccChannel);
  } catch (const std::out_of_range& oor) {
    alog->error("Response curve parameters for ABCStar on HCCStar input channel {} are not available", hccChannel);
    return -66;
  }

  return m_fitFunction(charge, params.data());
}