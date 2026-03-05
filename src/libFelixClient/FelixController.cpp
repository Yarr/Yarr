#ifdef YARR_CONFIG_FELIX_PROXY
#include "felix_proxy/ClientThread.h"
#else
#include "felix/felix_client_thread.hpp"
#endif
#include "felix/felix_client_properties.h"

#include "AllHwControllers.h"
#include "FelixController.h"

#include "Utils.h"

#include "logging.h"

namespace {
  auto fclog = logging::make_log("FelixController");
}

void FelixController::loadConfig(const json &j) {

  // FelixClientThread configuration
  auto clientCfg = j["FelixClient"];

  FelixClientThread::ConfigV2 fcConfig;
  // Properties
  // See https://gitlab.cern.ch/atlas-tdaq-felix/felix-interface/-/blob/master/felix/felix_client_properties.h
  fcConfig.property[FELIX_CLIENT_LOCAL_IP_OR_INTERFACE] = clientCfg["localIPorInterface"];
  fcConfig.property[FELIX_CLIENT_BUS_DIR] = clientCfg["busDir"];
  fcConfig.property[FELIX_CLIENT_BUS_GROUP_NAME] = clientCfg["busGroupName"];
  fcConfig.property[FELIX_CLIENT_VERBOSE_BUS] = clientCfg["verboseBus"] ? "True" : "False";
  fcConfig.property[FELIX_CLIENT_USE_ASIO_EVLOOP] = clientCfg["useASIOevloop"]  ? "True" : "False";
  fcConfig.property[FELIX_CLIENT_USE_THREAD_UNSAFE_NETIO] = clientCfg["useThreadUnsafeNetio"]  ? "True" : "False"; 
      try {
    auto txCfg = j["ToFLX"];
    FelixTxCore::loadConfig(txCfg);
    FelixTxCore::setClient(fcConfig);
  } catch (std::runtime_error &je) {
    fclog->error("Failed to load FelixTxCore config");
    throw je;
  }
  try {
    auto rxCfg = j["ToHost"];
    FelixRxCore::loadConfig(rxCfg);
    FelixRxCore::setClient(fcConfig);
  } catch (std::runtime_error &je) {
    fclog->error("Failed to load FelixRxCore config");
    throw je;
  }

  // If the config contains any optoboard devices, add them to the list
  if (j.contains("OptoDevices")){
    size_t n_devs = j["OptoDevices"].size();
    for(size_t i=0; i < n_devs; i++){
      const json& dev = j["OptoDevices"][i];

      // assume defaults, set to any provided values
      uint8_t version = OptoUtils::DEFAULT_LPGBT_VERSION;
      uint16_t i2c_addr = OptoUtils::DEFAULT_I2C_ADDR;
      uint16_t dev_addr = OptoUtils::DEFAULT_LPGBT_PRIMARY_ADDR;
      uint16_t dev_primary_addr = OptoUtils::DEFAULT_LPGBT_PRIMARY_ADDR;
      std::string type = "lpgbt";
      uint64_t tx_fid = FelixTxCore::ic_fid_from_channel(0);
      uint64_t rx_fid = FelixRxCore::ic_fid_from_channel(0);

      if (dev.contains("version"))
        version = dev["version"];
      if (dev.contains("i2cAddr"))
        i2c_addr = dev["i2cAddr"];
      if (dev.contains("devAddr"))
        dev_addr = dev["devAddr"];
      if (dev.contains("devPrimaryAddr"))
        dev_primary_addr = dev["devPrimaryAddr"];
      if (dev.contains("type"))
        type = dev["type"];
      if (dev.contains("txFid")){
        std::string tx_fid_str = dev["txFid"];
        tx_fid = std::stoull(tx_fid_str, 0, 0);
      }
      if (dev.contains("rxFid")){
        std::string rx_fid_str = dev["rxFid"];
        rx_fid = std::stoull(rx_fid_str, 0, 0);
      }

      m_opto_dev_list.emplace_back(std::make_unique<OptoDevice>(version, i2c_addr, dev_addr, dev_primary_addr, type, tx_fid, rx_fid));
    }
  }

  json j_status = getStatus();
  std::istringstream ss(j_status.dump(4));
  std::string line;

  fclog->info("Controller status:");
  while(std::getline(ss, line)) {
    fclog->info(" ~~~ {}", line);
  }
}

const json FelixController::getStatus() {
  fclog->debug("getStatus");
  json j_status;

  uint64_t reg_value;

  // card type
  if ( readFwRegister("CARD_TYPE", reg_value) ) {
    switch (reg_value) {
    case 0x2c5:
      j_status["card_type"] = "FLX709";
      break;
    case 0x2c6:
      j_status["card_type"] = "FLX710";
      break;
    case 0x2c7:
      j_status["card_type"] = "FLX711";
      break;
    case 0x2c8:
      j_status["card_type"] = "FLX712";
      break;
    case 0x080:
      j_status["card_type"] = "FLX128";
      break;
    }
  }

  // register map version
  if ( readFwRegister("REG_MAP_VERSION", reg_value) ) {
    // 0xabcd => version ab.cd
    int major = (reg_value >> 8) & 0xff;
    int minor = reg_value & 0xff;
    j_status["register_map_version"] = std::to_string(major)+"."+std::to_string(minor);
  }

  
  // firmware git hash
  if ( readFwRegister("GIT_HASH", reg_value) ) {
    j_status["firmware_git_hash"] = Utils::hexify(reg_value);
  }

  /*
  This doesn't seem to be a problem anymore?
  The above would crash in client->send_cmd:
     terminate called after throwing an instance of 'simdjson::simdjson_error'
     what():  The JSON number is too large or too small to fit within the requested type.
  */

  // firmware git tag
  if ( readFwRegister("GIT_TAG", reg_value) ) {
    j_status["firmware_git_tag"] = Utils::hexify(reg_value);
  }

  // firmware mode
  if ( readFwRegister("FIRMWARE_MODE", reg_value) ) {
    switch (reg_value) {
    case 0:
      j_status["firmware_mode"] = "GBT mode";
      break;
    case 1:
      j_status["firmware_mode"] = "FULL mode";
      break;
    case 2:
      j_status["firmware_mode"] = "LTDB mode";
      break;
    case 3:
      j_status["firmware_mode"] = "FEI4 mode";
      break;
    case 4:
      j_status["firmware_mode"] = "ITK Pixel";
      break;
    case 5:
      j_status["firmware_mode"] = "ITK Strip";
      break;
    case 6:
      j_status["firmware_mode"] = "FELIG";
      break;
    case 7:
      j_status["firmware_mode"] = "FULL mode emulator";
      break;
    case 8:
      j_status["firmware_mode"] = "FELIX_MROD mode";
      break;
    case 9:
      j_status["firmware_mode"] = "lpGBT mode";
      break;
    case 10:
      j_status["firmware_mode"] = "25G Interlaken";
      break;
    }
  }

  // XADC temperature monitor for the FPGA CORE
  if ( readFwRegister("FPGA_CORE_TEMP", reg_value) ) {
    float temp_C = ((reg_value* 502.9098)/4096)-273.8195;
    j_status["fpga_core_temperature"] = temp_C;
  }

  return j_status;
}

bool FelixController::readFwRegister(const std::string& registerName, uint64_t& value) {
  return FelixTxCore::readFwRegister(registerName, value);
}

bool FelixController::writeFwRegister(const std::string& registerName, const uint64_t& regValue) {
  return FelixTxCore::writeFwRegister(registerName, regValue);
}

// E-link control
bool FelixController::getICEnable(uint64_t fid) {
  fclog->debug("Get FID 0x{:x} IC channel enable:", fid);
  return checkRegValue(FelixTools::getICEnableRegName(fid), 1);
}

bool FelixController::getICEnable(const std::vector<uint64_t>& fids) {
  std::map<std::string, unsigned>  regICEnables;

  std::stringstream ss_fids;
  ss_fids << std::hex;

  for (const auto& fid : fids) {
    if (fclog->should_log(spdlog::level::debug)) ss_fids << " 0x" << fid;
    updateRegMap(regICEnables, FelixTools::getICEnableRegName(fid), 1, true);
  }

  fclog->debug("Get IC enables corresponding to FIDs:{}", ss_fids.str());
  return checkRegValuesAll(regICEnables);
}

bool FelixController::getICEnableExclusive(const std::vector<uint64_t>& fids) {
  std::map<std::string, unsigned>  regICEnables;

  // Initialize the register map with all possible registers
  bool allToFlx = FelixTools::all_toflx_from_fids(fids);
  bool allToHost = FelixTools::all_tohost_from_fids(fids);
  initAllICEnableRegMap(regICEnables, allToFlx, allToHost);

  std::stringstream ss_fids;
  ss_fids << std::hex;

  for (const auto& fid : fids) {
    if (fclog->should_log(spdlog::level::debug)) ss_fids << " 0x" << fid;
    updateRegMap(regICEnables, FelixTools::getICEnableRegName(fid), 1, true);
  }

  fclog->debug("Get IC enables corresponding to FIDs:{}", ss_fids.str());
  return checkRegValuesAll(regICEnables);
}

bool FelixController::getECEnable(uint64_t fid) {
  fclog->debug("Get FID 0x{:x} EC channel enable:", fid);
  return checkRegValue(FelixTools::getECEnableRegName(fid), 1);
}

bool FelixController::getECEnable(const std::vector<uint64_t>& fids) {
  std::map<std::string, unsigned>  regECEnables;

  std::stringstream ss_fids;
  ss_fids << std::hex;

  for (const auto& fid : fids) {
    if (fclog->should_log(spdlog::level::debug)) ss_fids << " 0x" << fid;
    updateRegMap(regECEnables, FelixTools::getECEnableRegName(fid), 1, true);
  }

  fclog->debug("Get EC enables corresponding to FIDs:{}", ss_fids.str());
  return checkRegValuesAll(regECEnables);
}

bool FelixController::getECEnableExclusive(const std::vector<uint64_t>& fids) {
  std::map<std::string, unsigned>  regECEnables;

  // Initialize the register map with all possible registers
  bool allToFlx = FelixTools::all_toflx_from_fids(fids);
  bool allToHost = FelixTools::all_tohost_from_fids(fids);
  initAllECEnableRegMap(regECEnables, allToFlx, allToHost);

  std::stringstream ss_fids;
  ss_fids << std::hex;

  for (const auto& fid : fids) {
    if (fclog->should_log(spdlog::level::debug)) ss_fids << " 0x" << fid;
    updateRegMap(regECEnables, FelixTools::getECEnableRegName(fid), 1, true);
  }

  fclog->debug("Get EC enables corresponding to FIDs:{}", ss_fids.str());
  return checkRegValuesAll(regECEnables);
}

bool FelixController::getELinkEnable(uint64_t fid) {
  fclog->debug("Get FID 0x{:x} enable:", fid);

  auto [linkId, egroup, epath, toflx] = FelixTools::linkInfo_from_fid(fid, fwMode());
  std::string regName = FelixTools::getELinkEnableRegName(linkId, egroup, toflx);
  unsigned value = (1 << epath);
  unsigned mask = value;

  return checkRegValue(regName, value, mask);
}

bool FelixController::getELinkEnable(const std::vector<uint64_t>& fids) {
  std::map<std::string, unsigned> enableRegMask;

  std::stringstream ss_fids;
  ss_fids << std::hex;

  for (const auto& fid : fids) {
    if (fclog->should_log(spdlog::level::debug)) ss_fids << " 0x" << fid;

    auto [linkId, egroup, epath, toflx] = FelixTools::linkInfo_from_fid(fid, fwMode());
    std::string regName = FelixTools::getELinkEnableRegName(linkId, egroup, toflx);
    unsigned mask = (1 << epath);

    updateRegMap(enableRegMask, regName, mask, false);
  }
  fclog->debug("Get enable of FIDs:{}", ss_fids.str());

  // only check the bits corresponding to the fids
  return checkRegValuesAll(enableRegMask, enableRegMask);
}

bool FelixController::getELinkEnableExclusive(const std::vector<uint64_t>& fids) {
  std::map<std::string, unsigned> enableRegMask;

  // Initialize the register map with all possible registers
  bool allToFlx = FelixTools::all_toflx_from_fids(fids);
  bool allToHost = FelixTools::all_tohost_from_fids(fids);
  initAllELinkEnableRegMap(enableRegMask, allToFlx, allToHost);

  std::stringstream ss_fids;
  ss_fids << std::hex;

  for (const auto& fid : fids) {
    if (fclog->should_log(spdlog::level::debug)) ss_fids << " 0x" << fid;

    auto [linkId, egroup, epath, toflx] = FelixTools::linkInfo_from_fid(fid, fwMode());
    std::string regName = FelixTools::getELinkEnableRegName(linkId, egroup, toflx);
    unsigned mask = (1 << epath);

    updateRegMap(enableRegMask, regName, mask, false);
  }
  fclog->debug("Get enable of FIDs:{}", ss_fids.str());

  // compare all registers and check they match exactly
  return checkRegValuesAll(enableRegMask, 0);
}

unsigned FelixController::getELinkWidthNBits(uint64_t fid) {
  fclog->debug("Get FID 0x{:x} width:", fid);

  unsigned nbits_w {0};

  std::string regName = FelixTools::getELinkWidthRegName(fid, fwMode());
  uint64_t regValue;
  if ( readFwRegister(regName, regValue) ) {
    fclog->debug(" {} = 0x{:x}", regName, regValue);
    nbits_w = 2 << (regValue & 0x7);
  }
  /* else {
    failed to read the register
  }*/
  return nbits_w;
}

unsigned FelixController::getELinkWidthMbps(uint64_t fid) {
  return getELinkWidthNBits(fid) * 40;
}

//////
bool FelixController::setICEnable(uint64_t fid, bool enable) {
  fclog->debug("Set FID 0x{:x} IC channel enable:", fid);
  return setRegValue(FelixTools::getICEnableRegName(fid), enable);
}

bool FelixController::setICEnable(const std::vector<uint64_t>& fids, bool enable) {
  std::map<std::string, unsigned> regICEnables;

  std::stringstream ss_fids;
  ss_fids << std::hex;

  for (unsigned i=0; i<fids.size(); ++i) {
    if (fclog->should_log(spdlog::level::debug)) ss_fids << " 0x" << fids[i];
    updateRegMap(regICEnables, FelixTools::getICEnableRegName(fids[i]), enable, true);
  }

  fclog->debug("Set IC enables corresponding to FIDs:{}", ss_fids.str());
  return setRegValueAll(regICEnables);
}

bool FelixController::setICEnableExclusive(const std::vector<uint64_t>& fids) {
  std::map<std::string, unsigned> regICEnables;

  // Initialize the register map with all possible registers
  bool allToFlx = FelixTools::all_toflx_from_fids(fids);
  bool allToHost = FelixTools::all_tohost_from_fids(fids);
  initAllICEnableRegMap(regICEnables, allToFlx, allToHost);

  std::stringstream ss_fids;
  ss_fids << std::hex;

  for (unsigned i=0; i<fids.size(); ++i) {
    if (fclog->should_log(spdlog::level::debug)) ss_fids << " 0x" << fids[i];
    updateRegMap(regICEnables, FelixTools::getICEnableRegName(fids[i]), true, true);
  }

  fclog->debug("Set IC enables corresponding to FIDs:{}", ss_fids.str());
  return setRegValueAll(regICEnables);
}

bool FelixController::setECEnable(uint64_t fid, bool enable) {
  fclog->debug("Set FID 0x{:x} EC channel enable:", fid);
  return setRegValue(FelixTools::getECEnableRegName(fid), enable);
}

bool FelixController::setECEnable(const std::vector<uint64_t>& fids, bool enable) {
  std::map<std::string, unsigned> regECEnables;

  std::stringstream ss_fids;
  ss_fids << std::hex;

  for (unsigned i=0; i<fids.size(); ++i) {
    if (fclog->should_log(spdlog::level::debug)) ss_fids << " 0x" << fids[i];
    updateRegMap(regECEnables, FelixTools::getECEnableRegName(fids[i]), enable, true);
  }

  fclog->debug("Set EC enables corresponding to FIDs:{}", ss_fids.str());
  return setRegValueAll(regECEnables);
}

bool FelixController::setECEnableExclusive(const std::vector<uint64_t>& fids) {
  std::map<std::string, unsigned> regECEnables;

  // Initialize the register map with all possible registers
  bool allToFlx = FelixTools::all_toflx_from_fids(fids);
  bool allToHost = FelixTools::all_tohost_from_fids(fids);
  initAllICEnableRegMap(regECEnables, allToFlx, allToHost);

  std::stringstream ss_fids;
  ss_fids << std::hex;

  for (unsigned i=0; i<fids.size(); ++i) {
    if (fclog->should_log(spdlog::level::debug)) ss_fids << " 0x" << fids[i];
    updateRegMap(regECEnables, FelixTools::getECEnableRegName(fids[i]), true, true);
  }

  fclog->debug("Set EC enables corresponding to FIDs:{}", ss_fids.str());
  return setRegValueAll(regECEnables);
}

bool FelixController::setELinkEnable(uint64_t fid, bool enable) {
  fclog->debug("Set FID 0x{:x} enable to {}", fid, enable);
  auto [linkId, egroup, epath, toflx] = FelixTools::linkInfo_from_fid(fid, fwMode());
  auto regName = FelixTools::getELinkEnableRegName(linkId, egroup, toflx);

  return setRegValue(regName, enable << epath, 1 << epath);
}

bool FelixController::setELinkEnable(const std::vector<uint64_t>& fids, bool enable) {
  std::map<std::string, unsigned> enableRegValue;
  std::map<std::string, unsigned> enableRegMask;

  std::stringstream ss_fids;
  ss_fids << std::hex;

  for (unsigned i=0; i<fids.size(); ++i) {
    if (fclog->should_log(spdlog::level::debug)) ss_fids << " 0x" << fids[i];

    auto [linkId, egroup, epath, toflx] = FelixTools::linkInfo_from_fid(fids[i], fwMode());
    auto regName = FelixTools::getELinkEnableRegName(linkId, egroup, toflx);

    updateRegMap(enableRegValue, regName, enable << epath, false);
    updateRegMap(enableRegMask, regName, 1 << epath, false);
  }

  fclog->debug("Set enable of FIDs:{}", ss_fids.str());
  return setRegValueAll(enableRegValue, enableRegMask);
}

bool FelixController::setELinkEnableExclusive(const std::vector<uint64_t>& fids) {
  std::map<std::string, unsigned> enableRegValue;

  // Initialize the register map with all possible registers
  bool allToFlx = FelixTools::all_toflx_from_fids(fids);
  bool allToHost = FelixTools::all_tohost_from_fids(fids);
  initAllELinkEnableRegMap(enableRegValue, allToFlx, allToHost);

  std::stringstream ss_fids;
  ss_fids << std::hex;

  for (unsigned i=0; i<fids.size(); ++i) {
    if (fclog->should_log(spdlog::level::debug)) ss_fids << " 0x" << fids[i];

    auto [linkId, egroup, epath, toflx] = FelixTools::linkInfo_from_fid(fids[i], fwMode());
    auto regName = FelixTools::getELinkEnableRegName(linkId, egroup, toflx);

    updateRegMap(enableRegValue, regName, 1 << epath, false);
  }

  fclog->debug("Set enable of FIDs:{}", ss_fids.str());
  return setRegValueAll(enableRegValue, 0);
}

bool FelixController::disableAllICs(bool toflx) {
  fclog->debug("Disabling all IC ({}) channels...", toflx?"Tx":"Rx");
  bool allGood {true};
  for (const auto& regName : FelixTools::getAllICEnableRegNames(toflx)) {
    allGood &= setRegValue(regName, 0);
  }
  return allGood;
}

bool FelixController::disableAllECs(bool toflx) {
  fclog->debug("Disabling all EC ({}) channels...", toflx?"Tx":"Rx");
  bool allGood {true};
  for (const auto& regName : FelixTools::getAllECEnableRegNames(toflx)) {
    allGood &= setRegValue(regName, 0);
  }
  return allGood;
}

bool FelixController::disableAllELinks(bool toflx) {
  fclog->debug("Disabling all e-links ({})...", toflx?"Tx":"Rx");
  bool allGood {true};
  for (const auto& regName : FelixTools::getAllELinkEnableRegNames(toflx)) {
    allGood &= setRegValue(regName, 0);
  }
  return allGood;
}

bool FelixController::setELinkWidthNBits(uint64_t fid, unsigned nbits) {
  fclog->debug("Set FID 0x{:x} width to {} bits", fid, nbits);

  std::string regName = FelixTools::getELinkWidthRegName(fid, fwMode());
  unsigned regValue = (__builtin_ctz(nbits) - 1) & 0x7; // 3 bits
  // or std::countr_zero when we move to c++20

  return setRegValue(regName, regValue);
}

bool FelixController::setELinkWidthNBits(const std::vector<uint64_t>& fids, unsigned nbits) {
  std::map<std::string, unsigned> regWidths;

  std::stringstream ss_fids;
  ss_fids << std::hex;

  for (const auto& fid : fids) {
    if (fclog->should_log(spdlog::level::debug)) ss_fids << " 0x" << fid;
    auto regName = FelixTools::getELinkWidthRegName(fid, fwMode());
    updateRegMap(regWidths, regName, nbits, true);
  }

  fclog->debug("Set e-link width to {} bits for FIDs:{}", nbits, ss_fids.str());
  return setRegValueAll(regWidths);
}

bool FelixController::setELinkWidthMbps(uint64_t fid, unsigned bandwidth) {
  fclog->debug("Set FID 0x{:x} width to {} Mbps", fid, bandwidth);
  return setELinkWidthNBits(fid, bandwidth/40);
}

bool FelixController::setELinkWidthMbps(const std::vector<uint64_t>& fids, unsigned bandwidth) {
  fclog->debug("Set e-link width to {} Mbps", bandwidth);
  return setELinkWidthNBits(fids, bandwidth/40);
}

void FelixController::updateRegMap(std::map<std::string, unsigned>& regMap, const std::string& regName, unsigned value, bool overwrite) {
  // check if regName is already in the map
  if (regMap.find(regName) != regMap.end()) {
    // already exist
    if (overwrite) {
      regMap[regName] = value;
    } else {
      regMap[regName] = regMap[regName] | value;
    }
  } else {
    //new key
    regMap[regName] = value;
  }
}

bool FelixController::checkRegValue(const std::string& regName, unsigned value, unsigned mask) {
  bool good {false};

  // read the register
  uint64_t regValue;
  if ( readFwRegister(regName, regValue) ) {
    // check against value
    if (mask) {
      // only compare the bits masked by mask to value
      good = (regValue & mask) == value;
    } else {
      // check if exactly the same
      good = regValue == value;
    }

    std::stringstream ss_log;
    ss_log << " " << regName << " = " << std::hex << "0x" << regValue << " (";
    if (mask) ss_log << "mask: 0x" << mask <<" ";
    ss_log << "expected value: 0x" << value << ")";

    if (good) {
      fclog->debug(ss_log.str());
    } else {
      fclog->warn(ss_log.str());
    }
  }
  // else failed to read FELIX register

  return good;
}

bool FelixController::checkRegValuesAll(const std::map<std::string, unsigned>& regValueMap, const std::map<std::string, unsigned>& regMaskMap) {
  // check map size
  if (regValueMap.size() != regMaskMap.size()) {
    fclog->error("Failed to set register values: value and mask maps are not of the same size!");
    return false;
  }

  bool allGood = true;

  for (const auto& [regName, value]: regValueMap) {
    auto regMask = regMaskMap.at(regName);
    allGood &= checkRegValue(regName, value, regMask);
  }

  if (not allGood) {
    fclog->warn(" Not all registers have the expected value!");
  }
  return allGood;
}

bool FelixController::checkRegValuesAll(const std::map<std::string, unsigned>& regValueMap, unsigned mask) {
  bool allGood = true;

  for (const auto& [regName, value]: regValueMap) {
    allGood &= checkRegValue(regName, value, mask);
  }

  if (not allGood) {
    fclog->warn(" Not all registers have the expected value!");
  }
  return allGood;
}

bool FelixController::setRegValue(const std::string& regName, unsigned value, unsigned mask) {
  uint64_t regValueOld, regValueNew;

  if (mask) {
    // read the register first
    bool readSuccess = readFwRegister(regName, regValueOld);

    if (not readSuccess) {
      fclog->error("Failed to set register {}: cannot access its current value", regName);
      return false;
    }

    // modify only the bits corresponding to the mask
    regValueNew = (regValueOld & ~mask) | (value & mask);
  } else {
    // Special case if mask is 0: overwrite the old value
    regValueNew = value;
  }

  bool writeSuccess = writeFwRegister(regName, regValueNew);
  if (writeSuccess) {
    if (mask) {
      fclog->debug(" {} = 0x{:x} (old value: 0x{:x})", regName, regValueNew, regValueOld);
    } else {
      fclog->debug(" {} = 0x{:x}", regName, regValueNew);
    }
  }

  return writeSuccess;
}

bool FelixController::setRegValueAll(const std::map<std::string, unsigned>& regValueMap, const std::map<std::string, unsigned>& regMaskMap) {
  // check map size
  if (regValueMap.size() != regMaskMap.size()) {
    fclog->error("Failed to set register values: value and mask maps are not of the same size!");
    return false;
  }

  bool allSuccess = true;

  for (const auto& [regName, regValue]: regValueMap) {
    auto regMask = regMaskMap.at(regName);
    allSuccess &= setRegValue(regName, regValue, regMask);
  }

  if (not allSuccess) {
    fclog->warn(" Not all register values are updated!");
  }
  return allSuccess;
}

bool FelixController::setRegValueAll(const std::map<std::string, unsigned>& regValueMap, unsigned mask) {
  bool allSuccess = true;
  for (const auto& [regName, regValue]: regValueMap) {
    allSuccess &= setRegValue(regName, regValue, mask);
  }

  if (not allSuccess) {
    fclog->warn(" Not all register values are updated!");
  }
  return allSuccess;
}

void FelixController::initAllICEnableRegMap(std::map<std::string, unsigned>& regMap, bool toflx, bool tohost) {
  for (const auto& regName : FelixTools::getAllICEnableRegNames(toflx)) {
    regMap[regName] = 0;
  }

  if (toflx == tohost) {
    // mixed direction, add register names for the opposite direction too
    for (const auto& regName : FelixTools::getAllICEnableRegNames(not toflx)) {
      regMap[regName] = 0;
    }
  }
}

void FelixController::initAllECEnableRegMap(std::map<std::string, unsigned>& regMap, bool toflx, bool tohost) {
  for (const auto& regName : FelixTools::getAllECEnableRegNames(toflx)) {
    regMap[regName] = 0;
  }

  if (toflx == tohost) {
    // mixed direction, add register names for the opposite direction too
    for (const auto& regName : FelixTools::getAllECEnableRegNames(not toflx)) {
      regMap[regName] = 0;
    }
  }
}

void FelixController::initAllELinkEnableRegMap(std::map<std::string, unsigned>& regMap, bool toflx, bool tohost) {
  for (const auto& regName : FelixTools::getAllELinkEnableRegNames(toflx)) {
    regMap[regName] = 0;
  }

  if (toflx == tohost) {
    // mixed direction, add register names for the opposite direction too
    for (const auto& regName : FelixTools::getAllELinkEnableRegNames(not toflx)) {
      regMap[regName] = 0;
    }
  }
}

/*
Optoboard communication functions
*/
bool FelixController::optoDeviceInList(uint64_t rx_ic_fid, uint16_t dev_addr){
  bool in_list = false;
  for (const auto& dev : m_opto_dev_list){
    if (dev->getRxFid() == rx_ic_fid && dev->getDevAddr() == dev_addr){
      in_list = true;
      break;
    }
  }
  return in_list;
}

// Note several devices may be listed with these properties, this function is mainly used to obtain the primary lpgbt address
// which will be common between the different devices that may share these fids
bool FelixController::optoDeviceInList(uint64_t rx_ic_fid, uint64_t tx_ic_fid, std::string type){
  bool in_list = false;
  for (const auto& dev : m_opto_dev_list){
    if (dev->getRxFid() == rx_ic_fid && dev->getTxFid() == tx_ic_fid && dev->getDevType() == type){
      in_list = true;
      fclog->debug("Device with rx ic fid 0x{:x}, tx ic fid 0x{:x} and type {} is in the list", rx_ic_fid, tx_ic_fid, type);
      break;
    }
  }
  return in_list;
}

FelixController::OptoDevice* FelixController::getOptoDeviceInList(uint64_t rx_ic_fid, uint16_t dev_addr){
  fclog->debug("Obtaining Opto Device with rx fid 0x{:x} and address 0x{:x}",rx_ic_fid, dev_addr);
  for (const auto& dev : m_opto_dev_list){
    if (dev->getRxFid() == rx_ic_fid && dev->getDevAddr() == dev_addr){
      return dev.get();
    }
  }
  fclog->error("Opto device not found in list, returning a nullpointer");
  return nullptr;
}

// Note several devices may be listed with these properties, this function is mainly used to obtain the primary lpgbt address
// which will be common between the different devices that may share these fids
uint16_t FelixController::getOptoDevPrimaryAddr(uint64_t rx_ic_fid, uint64_t tx_ic_fid, std::string type){
  fclog->debug("Obtaining Opto Device with rx fid 0x{:x}, tx fid 0x{:x}, and type {}",rx_ic_fid, tx_ic_fid, type);
  for (const auto& dev : m_opto_dev_list){
    if (dev->getRxFid() == rx_ic_fid && dev->getTxFid() == tx_ic_fid && dev->getDevType() == type){
      return (dev.get())->getPrimaryAddr();
    }
  }

  fclog->error("Opto device not found in list, returning the default");
  return OptoUtils::DEFAULT_LPGBT_PRIMARY_ADDR;
}

FelixController::OptoDevice* FelixController::newDefaultOptoDevice(uint16_t dev_addr, std::string type, uint64_t rx_ic_fid, uint64_t tx_ic_fid){
  // Make sure first it doesn't already exist
  if (!optoDeviceInList(rx_ic_fid, dev_addr)){
    m_opto_dev_list.emplace_back(std::make_unique<OptoDevice>(OptoUtils::DEFAULT_LPGBT_VERSION, OptoUtils::DEFAULT_I2C_ADDR, dev_addr, OptoUtils::DEFAULT_LPGBT_PRIMARY_ADDR, type, tx_ic_fid, rx_ic_fid));
    fclog->debug("Creating a new opto device with rx fid 0x{:x} and address 0x{:x}",rx_ic_fid, dev_addr);
  }
  // Print message if it already exists
  else {
    fclog->info(" Device already exists in the list!  Returning matching device");
  }
  // Return a raw pointer corresponding to the new default object
  return getOptoDeviceInList(rx_ic_fid, dev_addr);
}

void FelixController::communicateLpGBT(const lpgbt_item_t* reg, uint8_t& data, const bool write, OptoDevice* lpgbt){
  if (write){
    fclog->debug("Writing value {:x} to register {} with address 0x{:x} on device with address 0x{:x}", data, reg->name, reg->addr, lpgbt->getDevAddr());
  }
  else {
    fclog->debug("Reading value register {} with address 0x{:x} on device with address 0x{:x}", reg->name, reg->addr, lpgbt->getDevAddr());
  }
  std::vector<uint8_t> netio_frame;
  fclog->debug("Preparing netio frame");
  netio_frame = OptoUtils::prepareICDataFrame(write, reg->addr, data, lpgbt->getVersion(), lpgbt->getPrimaryAddr());
  
  // LpGBT registers have sub-fields with different bit indexes
  // we need these later to calculate the correct result but currently we need the main bit index
  // in order to properly parse the result from the register 
  const lpgbt_item_t* reg_primary = OptoUtils::getLpGBTRegisterByAddr(reg->addr, lpgbt->getVersion());

  if (fclog->should_log(spdlog::level::debug)){
    for (size_t i = 0; i < netio_frame.size(); i++){
      fclog->debug("netio frame at {} is {:x}", i, netio_frame[i]);
    }
  }

  unsigned int first_ic_payload_byte = 0;
  if (lpgbt->getVersion() == 0){
    first_ic_payload_byte = OptoUtils::FIRST_IC_PAYLOAD_BYTE_V0;
  }
  else {
    first_ic_payload_byte = OptoUtils::FIRST_IC_PAYLOAD_BYTE_V1;
  }

  int num_tries = 0;
  bool success = false;
  while (!success && num_tries < 4){
    try {
      FelixRxCore::flushBuffer();
      FelixRxCore::clearRawData();
      FelixTxCore::sendIC(lpgbt->getTxFid(), netio_frame);

      std::vector<RawDataPtr> reply = FelixRxCore::readData();
      while (reply.size() == 0){
        reply = FelixRxCore::readData();
        std::this_thread::sleep_for(std::chrono::microseconds(100));
      }
      FelixRxCore::flushBuffer();
      // reply is 64 bits long, saved in two buffers of size 32 bits
      // The first ic payload byte is either 6 or 7, depending on the version,
      // which is stored in the second buffer of the reply (reply[0]->get(1))
      if (reply.size() > 0){
        uint32_t index_32 = first_ic_payload_byte % 4; // find the index for the data
        uint32_t at_block = reply[0]->get(1); // get the data block
        //fclog->info("The data block we have is 0x{:x}",at_block);
        uint32_t mask = (0xFF << (8 * index_32)); // define the mask to get the value
        uint8_t tmpvalue = (mask & at_block) >> (8*index_32); // apply the mask
        uint8_t bitmask = (0xFF>>(8-reg_primary->nbits)) << reg_primary->bitindex;
        uint8_t value = tmpvalue & bitmask; // at this point, tmpvalue will be a uint8_t
        fclog->debug("The value is 0x{:x}", value);
        success = true;
        if (!write)
          data = value;
        else {
          if (data != value){
            fclog->error("Incorrect value written, wrote 0x{:x} instead of the provided value  0x{:x}", value, data);
            num_tries++;
            fclog->error("Trying again to write 0x{:x}, number of attempts taken: {}", data, num_tries);
          }
        }
      }
      else {
        fclog->error("No reply received when accessing register with address {} at fid {}", reg->addr, lpgbt->getTxFid());
      }
    }
    catch (std::runtime_error &e){
      fclog->error(e.what());
    }
  }
}

void FelixController::readWriteOptoReg(const lpgbt_item_t* reg , uint8_t& reg_data, bool write, OptoDevice* lpgbt){
  // Check if the fids are already enabled, if not, enable them
  try {
    if (!FelixRxCore::channelIsEnabled(lpgbt->getRxFid())){
      setICEnable(lpgbt->getRxFid());
      FelixRxCore::enableChannel(lpgbt->getRxFid());
    }
    if (!FelixTxCore::channelIsEnabled(lpgbt->getTxFid())){
      setICEnable(lpgbt->getTxFid());
      FelixTxCore::enableChannel(lpgbt->getTxFid());
    }
  }
  catch (std::runtime_error &e){
    fclog->error(e.what());
  }

  // if we're communicating directly to the primary LpGBT, we only need to send one simple register read
  if (lpgbt->isPrimary() && lpgbt->getDevType() == "lpgbt"){
    fclog->debug("Accessing primary LpGBT");
    communicateLpGBT(reg, reg_data, write, lpgbt);
  }

  // communicating with secondary LpGBTs or GBCRs via I2C channel through the primary LpGBT
  else {
    fclog->debug("Communicating over I2C");
    uint8_t NBYTE = 0;

    if (lpgbt->getDevType() == "lpgbt" && write){
      NBYTE = 3;
    }
    else if (lpgbt->getDevType() == "lpgbt" && !write){
      NBYTE = 2;
    }
    else if (lpgbt->getDevType() == "gbcr" && write){
      NBYTE = 2;
    }
    else if (lpgbt->getDevType() == "gbcr" && !write){
      NBYTE = 1;
    }
    
    std::string i2c_addr_str = "I2CM" + std::to_string(lpgbt->getI2CAddr());

    // Registers for i2c communication
    const lpgbt_item_t* data0 = OptoUtils::getLpGBTRegisterByName((i2c_addr_str+"DATA0").c_str(), lpgbt->getVersion());
    const lpgbt_item_t* cmd = OptoUtils::getLpGBTRegisterByName((i2c_addr_str+"CMD").c_str(), lpgbt->getVersion());
    const lpgbt_item_t* data1 = OptoUtils::getLpGBTRegisterByName((i2c_addr_str+"DATA1").c_str(), lpgbt->getVersion());
    const lpgbt_item_t* data2 = OptoUtils::getLpGBTRegisterByName((i2c_addr_str+"DATA2").c_str(), lpgbt->getVersion());
    const lpgbt_item_t* addr = OptoUtils::getLpGBTRegisterByName((i2c_addr_str+"ADDRESS").c_str(), lpgbt->getVersion());
    const lpgbt_item_t* status_reg = OptoUtils::getLpGBTRegisterByName((i2c_addr_str + "STATUS").c_str(), lpgbt->getVersion());
    const lpgbt_item_t* read15 = OptoUtils::getLpGBTRegisterByName((i2c_addr_str + "READ15").c_str(), lpgbt->getVersion());

    uint8_t data0_val = (OptoUtils::m_scldrive << 7) | (NBYTE << 2) | OptoUtils::m_freq;
    uint8_t cmd_val = OptoUtils::m_i2c_write_cr;
    
    communicateLpGBT(data0, data0_val, 1, lpgbt);
    communicateLpGBT(cmd, cmd_val, 1, lpgbt);

    data0_val = (reg->addr & 0x0FF);
    uint8_t data1_val = (reg->addr & 0xF00) >> 8;
    uint8_t data2_val = reg_data;
    if (lpgbt->getDevType() == "lpgbt"){
      // Send the address of the register we want to read, and data if we're writing it
      communicateLpGBT(data0, data0_val, 1, lpgbt);    //Lower half of register address
      communicateLpGBT(data1, data1_val, 1, lpgbt);    //Upper half of register address
      if (write){
        communicateLpGBT(data2, data2_val, 1, lpgbt); // register data
      }
    }
    else if (lpgbt->getDevType() == "gbcr"){
      data0_val = reg->addr;
      communicateLpGBT(data0, data0_val, 1, lpgbt);
      if (write){
        communicateLpGBT(data1, reg_data, 1, lpgbt);
      }
    }
    
    cmd_val = OptoUtils::m_i2c_w_multi_4byte0;
    uint8_t addr_val = lpgbt->getDevAddr();
    communicateLpGBT(cmd, cmd_val, 1, lpgbt);
    communicateLpGBT(addr, addr_val, 1, lpgbt);

    cmd_val = OptoUtils::m_i2c_write_multi;
    communicateLpGBT(cmd, cmd_val, 1, lpgbt) ;   //Initiate send of register address and register value

    // Read back answer
    NBYTE = 1;
    data0_val = (OptoUtils::m_scldrive << 7) | (NBYTE << 2) | OptoUtils::m_freq;
    cmd_val = OptoUtils::m_i2c_write_cr;
    communicateLpGBT(data0, data0_val, 1, lpgbt);
    communicateLpGBT(cmd, cmd_val, 1, lpgbt);
    communicateLpGBT(addr, addr_val, 1, lpgbt);

    cmd_val = OptoUtils::m_i2c_read_multi;
    communicateLpGBT(cmd, cmd_val, 1, lpgbt);

    uint8_t status = 0;
    communicateLpGBT(status_reg, status, 0, lpgbt);

    if (!status){
      fclog->error("I2C readback status failed: {} ", status);
    }

    // Read answer via from I2C communication
    communicateLpGBT(read15, reg_data, 0, lpgbt);
  }
}

bool FelixController::readLpGBTRegister(const char* reg_name, uint8_t& reg_data, uint16_t dev_addr, uint64_t rx_ic_fid, uint64_t tx_ic_fid){
  OptoDevice* lpgbt = nullptr;
  if (!optoDeviceInList(rx_ic_fid, dev_addr)){
    lpgbt = newDefaultOptoDevice(dev_addr, "lpgbt", rx_ic_fid, tx_ic_fid);
  }
  else {
    lpgbt = getOptoDeviceInList(rx_ic_fid, dev_addr);
  }

  const lpgbt_item_t* reg = OptoUtils::getLpGBTRegisterByName(reg_name, lpgbt->getVersion());

  uint8_t readback;
  try {
    readWriteOptoReg(reg, readback, 0, lpgbt);
  }
  catch (std::runtime_error &e){
    fclog->error(e.what());
    return false;
  }

  // If we're reading off a register that is a sub-field of another register we need to apply a mask to the response
  if (OptoUtils::regField(reg)){
    reg_data = OptoUtils::applyRegfieldReadMask(reg, readback);
  }
  else {
    reg_data = readback;
  }
  return true;
}

bool FelixController::writeLpGBTRegister(const char* reg_name, uint8_t reg_data, uint16_t dev_addr, uint64_t rx_ic_fid, uint64_t tx_ic_fid){
  OptoDevice* lpgbt = nullptr;
  if (!optoDeviceInList(rx_ic_fid, dev_addr)){
    lpgbt = newDefaultOptoDevice(dev_addr, "lpgbt", rx_ic_fid, tx_ic_fid);
  }
  else {
    lpgbt = getOptoDeviceInList(rx_ic_fid, dev_addr);
  }

  const lpgbt_item_t* reg = OptoUtils::getLpGBTRegisterByName(reg_name, lpgbt->getVersion());

  uint8_t write_data = reg_data;

  // If the register we're writing to is part of a register subfield, we need to make sure we're writing to the correct bits in the bitstream
  // The following is the procedure for this:
  if (OptoUtils::regField(reg)){
    // Get the full readout from the total register
    // const lpgbt_item_t* reg_primary = OptoUtils::getLpGBTRegisterByAddr(reg->addr, lpgbt->getVersion());

    uint8_t current_data = 0;
    try {
      readWriteOptoReg(reg, current_data, 0, lpgbt);
    }
    catch (std::runtime_error &e){
      fclog->error(e.what());
      return false;
    }
    write_data = OptoUtils::applyRegfieldWriteMask(reg, reg_data, current_data, lpgbt->getVersion());
  }

  try {
    readWriteOptoReg(reg, write_data, 1, lpgbt);
  }
  catch (std::runtime_error &e){
    fclog->error(e.what());
    return false;
  }
  return true;
}

bool felix_registered = StdDict::registerHwController(
  "FelixClient",
  []() {return std::unique_ptr<HwController>(new FelixController);}
  );
