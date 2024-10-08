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
bool FelixController::getICEnable(uint16_t linkId, bool toflx) {
  std::string regName = FelixTools::getICEnableRegName(linkId, toflx);
  uint64_t regValue;

  if ( readFelixRegister(regName, regValue) ) {
    return regValue & 1;
  } else {
    // failed to read the register
    return false;
  }
}

bool FelixController::getECEnable(uint16_t linkId, bool toflx) {
  std::string regName = FelixTools::getECEnableRegName(linkId, toflx);
  uint64_t regValue;

  if (readFelixRegister(regName, regValue) ) {
    return regValue & 1;
  } else {
    // failed to read the register
    return false;
  }
}

bool FelixController::getELinkEnable(unsigned chn, bool toflx) {
  auto [linkId, egroup, epath] = FelixTools::linkInfo_from_chn(chn);

  std::string regName = FelixTools::getELinkEnableRegName(linkId, egroup, toflx);
  uint64_t regValue;

  if ( readFelixRegister(regName, regValue) ) {
    // check the bit
    return regValue & (1 << epath);
  } else {
    // failed to read the register
    return false;
  }
}

bool FelixController::getELinkEnable(uint64_t fid) {
  auto [linkId, egroup, epath, toflx] = FelixTools::linkInfo_from_fid(fid);

  std::string regName = FelixTools::getELinkEnableRegName(linkId, egroup, toflx);
  uint64_t regValue;

  if (readFelixRegister(regName, regValue) ) {
    // check the bit
    return regValue & (1 << epath);
  } else {
    // failed to read the register
    return false;
  }
}

bool FelixController::getELinkEnablesAll(const std::vector<unsigned>& chns, bool toflx) {
  std::map<std::string, uint8_t> enableRegMask;

  for (const auto& chn : chns) {
    auto [linkId, egroup, epath] = FelixTools::linkInfo_from_chn(chn);
    updateEnableMap(enableRegMask, linkId, egroup, epath, toflx);
  }

  return checkELinkEnableRegs(enableRegMask);
}

bool FelixController::getELinkEnablesAll(const std::vector<uint64_t>& fids) {
  std::map<std::string, uint8_t> enableRegMask;

  for (const auto& fid : fids) {
    auto [linkId, egroup, epath, toflx] = FelixTools::linkInfo_from_fid(fid);
    updateEnableMap(enableRegMask, linkId, egroup, epath, toflx);
  }

  return checkELinkEnableRegs(enableRegMask);
}

bool FelixController::setICEnable(uint16_t linkId, bool toflx, bool enable) {
  return writeFelixRegister(
    FelixTools::getICEnableRegName(linkId, toflx), std::to_string(enable)
  );
}

bool FelixController::setECEnable(uint16_t linkId, bool toflx, bool enable) {
  return writeFelixRegister(
    FelixTools::getECEnableRegName(linkId, toflx), std::to_string(enable)
  );
}

bool FelixController::setELinkEnable(unsigned chn, bool toflx, bool enable) {
  auto [linkId, egroup, epath] = FelixTools::linkInfo_from_chn(chn);
  return setELinkEnableImpl(enable, linkId, egroup, epath, toflx);
}

bool FelixController::setELinkEnable(uint64_t fid, bool enable) {
  auto [linkId, egroup, epath, toflx] = FelixTools::linkInfo_from_fid(fid);
  return setELinkEnableImpl(enable, linkId, egroup, epath, toflx);
}

bool FelixController::setELinkEnables(const std::vector<unsigned> chns, bool toflx, const std::vector<bool>& enables) {
  // check if chns and enables are of the same length
  if (chns.size() != enables.size()) {
    fclog->error("Failed to set e-link enable registers: inconsistent numbers of channels and enables");
    return false;
  }

  std::map<std::string, uint8_t> enableRegMask;
  std::map<std::string, uint8_t> enableRegValue;

  for (unsigned i=0; i<chns.size(); ++i) {
    auto [linkId, egroup, epath] = FelixTools::linkInfo_from_chn(chns[i]);
    updateEnableMap(enableRegMask, linkId, egroup, epath, toflx);
    updateEnableMap(enableRegValue, linkId, egroup, epath, toflx, enables[i]);
  }

  return setELinkEnableRegs(enableRegMask, enableRegValue);
}

bool FelixController::setELinkEnables(const std::vector<unsigned> chns, bool toflx) {
  std::vector<bool> enables(chns.size(), true);
  return setELinkEnables(chns, toflx, enables);
}

bool FelixController::setELinkEnables(const std::vector<uint64_t> fids, const std::vector<bool>& enables) {
  // check if fids and enables are of the same length
  if (fids.size() != enables.size()) {
    fclog->error("Failed to set e-link enable registers: inconsistent numbers of channels and enables");
    return false;
  }

  std::map<std::string, uint8_t> enableRegMask;
  std::map<std::string, uint8_t> enableRegValue;

  for (unsigned i=0; i<fids.size(); ++i) {
    auto [linkId, egroup, epath, toflx] = FelixTools::linkInfo_from_fid(fids[i]);
    updateEnableMap(enableRegMask, linkId, egroup, epath, toflx);
    updateEnableMap(enableRegValue, linkId, egroup, epath, toflx, enables[i]);
  }

  return setELinkEnableRegs(enableRegMask, enableRegValue);
}

bool FelixController::setELinkEnables(const std::vector<uint64_t> fids) {
  std::vector<bool> enables(fids.size(), true);
  return setELinkEnables(fids, enables);
}

void FelixController::updateEnableMap(std::map<std::string, uint8_t>& maskMap, uint16_t linkId, uint8_t egroup, uint8_t epath, bool toflx, bool val) {
  std::string regName = FelixTools::getELinkEnableRegName(linkId, egroup, toflx);

  // check if regName is already in the map
  if (maskMap.find(regName) != maskMap.end()) {
    // already exist
    maskMap[regName] = maskMap[regName] | (val << epath);
  } else {
    // new key
    maskMap[regName] = (val << epath);
  }
}

bool FelixController::checkELinkEnableRegs(const std::map<std::string, uint8_t>& maskMap) {
  bool allGood = true;

  for (const auto& [regName, mask]: maskMap) {
    // read the register
    uint64_t regValue;
    if ( readFelixRegister(regName, regValue) ) {
      // check against mask
      bool regGood = (regValue & mask) == mask;
      if (not regGood) {
        fclog->warn("E-link enable register {} is expected to be 0x{:x} but is actually 0x{:x}.", regName, mask, regValue);
      }

      allGood &= regGood;
    } else {
      // failed to read the register
      allGood = false;
    }
  }

  return allGood;
}

bool FelixController::setELinkEnableImpl(bool enable, uint16_t linkId, uint8_t egroup, uint8_t epath, bool toflx) {
  std::string regName = FelixTools::getELinkEnableRegName(linkId, egroup, toflx);

  // read the register first
  uint64_t regValue;
  bool readSuccess = readFelixRegister(regName, regValue);

  if (not readSuccess) {
    fclog->error("Failed to set E-link enable: cannot access the current value of register {}", regName);
    return false;
  }

  // modify only the bit according to epath
  if (enable) {
    regValue |= (1 << epath);
  } else {
    regValue &= ~(1 << epath);
  }

  return writeFelixRegister(regName, std::to_string(regValue));
}

bool FelixController::setELinkEnableRegs(
  const std::map<std::string, uint8_t>& maskMap,
  const std::map<std::string, uint8_t>& valMap)
{
  // check map size
  if (maskMap.size() != valMap.size()) {
    fclog->error("Failed to set e-link enable registers: e-link enable mask and value maps are not of the same size!");
    return false;
  }

  bool writeSuccess = true;

  for (const auto& [regName, mask]: maskMap) {
    // read the current value
    uint64_t regValue;
    bool readSuccess = readFelixRegister(regName, regValue);
    if (not readSuccess) {
      fclog->error("Failed to set E-link enable: cannot access the current value of register {}", regName);
      return false;
    }

    // update the register value
    // only modify the bits according to mask to valMap[regName]
    uint64_t newValue = (regValue & ~mask) | (valMap.at(regName) & mask);

    writeSuccess &= writeFelixRegister(regName, std::to_string(newValue));
  }

  return writeSuccess;
}

bool felix_registered = StdDict::registerHwController(
  "FelixClient",
  []() {return std::unique_ptr<HwController>(new FelixController);}
  );
