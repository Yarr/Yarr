#include "AllHwControllers.h"
#include "FelixController.h"

#include "Utils.h"

#include "logging.h"

#include "felix/felix_client_properties.h"

namespace {
  auto fclog = logging::make_log("FelixController");
}

void FelixController::loadConfig(const json &j) {

  try {
    FelixClientThread::Config fcConfig;

    // Callbacks
    fcConfig.on_init_callback = [this]() {
      FelixController::on_init();
    };

    fcConfig.on_data_callback = [this](uint64_t fid, const uint8_t* data, size_t size, uint8_t status) {
      FelixController::on_data(fid, data, size, status);
    };

    fcConfig.on_connect_callback = [this](uint64_t fid) {
      FelixController::on_connect(fid);
    };

    fcConfig.on_disconnect_callback = [this](uint64_t fid) {
      FelixController::on_disconnect(fid);
    };

    auto clientCfg = j["FelixClient"];

    // Properties
    // See https://gitlab.cern.ch/atlas-tdaq-felix/felix-interface/-/blob/master/felix/felix_client_properties.h
    fcConfig.property[FELIX_CLIENT_LOCAL_IP_OR_INTERFACE] = clientCfg["localIPorInterface"];
    fcConfig.property[FELIX_CLIENT_LOG_LEVEL] = clientCfg["logLevel"];
    fcConfig.property[FELIX_CLIENT_BUS_DIR] = clientCfg["busDir"];
    fcConfig.property[FELIX_CLIENT_BUS_GROUP_NAME] = clientCfg["busGroupName"];
    fcConfig.property[FELIX_CLIENT_VERBOSE_BUS] = clientCfg["verboseBus"] ? "True" : "False";
    fcConfig.property[FELIX_CLIENT_TIMEOUT] = std::to_string(unsigned(clientCfg["timeout"]));
    fcConfig.property[FELIX_CLIENT_NETIO_PAGES] = std::to_string(unsigned(clientCfg["netioPages"]));
    fcConfig.property[FELIX_CLIENT_NETIO_PAGESIZE] = std::to_string(unsigned(clientCfg["netioPagesize"]));

    // Construct felix client
    client = std::make_shared<FelixClientThread>(fcConfig);

  } catch (std::runtime_error &fce) {
    fclog->error("Failed to construct Felix client");
    throw fce;
  } 

  try {
    auto txCfg = j["ToFLX"];
    FelixTxCore::setClient(client);
    FelixTxCore::loadConfig(txCfg);
  } catch (std::runtime_error &je) {
    fclog->error("Failed to load FelixTxCore config");
    throw je;
  }

  try {
    auto rxCfg = j["ToHost"];
    FelixRxCore::setClient(client);
    FelixRxCore::loadConfig(rxCfg);
  } catch (std::runtime_error &je) {
    fclog->error("Failed to load FelixRxCore config");
    throw je;
  }
}

const json FelixController::getStatus() {
  fclog->debug("getStatus");
  json j_status;

  uint64_t reg_value;
  bool read_good = false;

  // card type
  if ( readFelixRegister("CARD_TYPE", reg_value) ) {
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
  if ( readFelixRegister("REG_MAP_VERSION", reg_value) ) {
    // 0xabcd => version ab.cd
    int major = (reg_value >> 8) & 0xff;
    int minor = reg_value & 0xff;
    j_status["register_map_version"] = std::to_string(major)+"."+std::to_string(minor);
  }

  /*
  // firmware git hash
  if ( readFelixRegister("GIT_HASH", reg_value) ) {
    j_status["firmware_git_hash"] = Utils::hexify(reg_value);
  }

  The above would crash in client->send_cmd:
     terminate called after throwing an instance of 'simdjson::simdjson_error'
     what():  The JSON number is too large or too small to fit within the requested type.
  */

  // firmware git tag
  if ( readFelixRegister("GIT_TAG", reg_value) ) {
    j_status["firmware_git_tag"] = Utils::hexify(reg_value);
  }

  // firmware mode
  if ( readFelixRegister("FIRMWARE_MODE", reg_value) ) {
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
  if ( readFelixRegister("FPGA_CORE_TEMP", reg_value) ) {
    float temp_C = ((reg_value* 502.9098)/4096)-273.8195;
    j_status["fpga_core_temperature"] = temp_C;
  }

  return j_status;
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
  if ( readFelixRegister(regName, regValue) ) {
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
  fclog->debug("Set FID 0x{:x} width to {} bits", nbits);

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
  if ( readFelixRegister(regName, regValue) ) {
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
    bool readSuccess = readFelixRegister(regName, regValueOld);

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

  bool writeSuccess = writeFelixRegister(regName, std::to_string(regValueNew));
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


bool felix_registered = StdDict::registerHwController(
  "FelixClient",
  []() {return std::unique_ptr<HwController>(new FelixController);}
  );
