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
    FelixTxCore::loadConfig(txCfg);
    FelixTxCore::setClient(client);
  } catch (std::runtime_error &je) {
    fclog->error("Failed to load FelixTxCore config");
    throw je;
  }

  try {
    auto rxCfg = j["ToHost"];
    FelixRxCore::loadConfig(rxCfg);
    FelixRxCore::setClient(client);
  } catch (std::runtime_error &je) {
    fclog->error("Failed to load FelixRxCore config");
    throw je;
  }
}

FelixClientThread::Reply FelixController::accessFelixRegister(
  FelixClientThread::Cmd cmd, const std::vector<std::string>& cmd_args)
{
  // A dummy fid made from the correct did and cid, but arbitrary link number
  // send_cmd will map this to the proper fid for register access
  std::vector<uint64_t> fids = {FelixTxCore::fid_from_channel(42)};

  // felix-register can potentially serve multiple devices
  std::vector<FelixClientThread::Reply> replies;

  auto status_summary = client->send_cmd(fids, cmd, cmd_args, replies);

  if (replies.empty()) {
    fclog->warn("Status: {}", FelixClientThread::to_string(status_summary));
    throw std::runtime_error("No replies.");
  }

  // The current setup assumes the controller only handles one FELIX device (with m_did and m_cid)
  // replies.size() should also be the same as fids.size() for send_cmd()
  assert(replies.size()==1);
  const auto& reply = replies[0];

  return reply;
}

bool FelixController::checkReply(const FelixClientThread::Reply& reply) {

  bool goodReply = reply.status == FelixClientThread::Status::OK;

  if (not goodReply) {
    fclog->warn("Status: {}", FelixClientThread::to_string(reply.status));
    fclog->warn(reply.message);
  } else {
    //status OK
    fclog->debug("OK from 0x{:x}", reply.ctrl_fid);
    fclog->debug("Register value = 0x{:x}", reply.value);
    if (not reply.message.empty()) fclog->debug("message: {}", reply.message);
  }

  return goodReply;
}

bool FelixController::readFelixRegister(
  const std::string& registerName, uint64_t& value)
{
  fclog->debug("Read FELIX register {}", registerName);

  bool success = false;

  try {
    auto reply = accessFelixRegister(FelixClientThread::Cmd::GET, {registerName});
    success = checkReply(reply);
    value = reply.value;
  } catch (std::runtime_error &e) {
    fclog->error(e.what());
  }

  if (not success) {
    fclog->error("Fail to read register {}", registerName);
  }

  return success;
}

bool FelixController::writeFelixRegister(
  const std::string& registerName, const std::string& regValue
)
{
  fclog->debug("Write value {} to FELIX register {}", regValue, registerName);

  bool success = false;

  try {
    auto reply = accessFelixRegister(FelixClientThread::Cmd::SET, {registerName, regValue});
    success = checkReply(reply);
  } catch (std::runtime_error &e) {
    fclog->error(e.what());
  }

  if (not success) {
    fclog->error("Fail to write register {}", registerName);
  }

  return success;
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

bool felix_registered = StdDict::registerHwController(
  "FelixClient",
  []() {return std::unique_ptr<HwController>(new FelixController);}
  );
