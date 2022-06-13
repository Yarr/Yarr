#include "AllHwControllers.h"
#include "FelixController.h"

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
    // See https://gitlab.cern.ch/atlas-tdaq-felix/felix-interface/-/blob/4.2.x/felix/felix_client_properties.h
    fcConfig.property[FELIX_CLIENT_LOCAL_IP_OR_INTERFACE] = clientCfg["local_ip_or_interface"];
    fcConfig.property[FELIX_CLIENT_LOG_LEVEL] = clientCfg["log_level"];
    fcConfig.property[FELIX_CLIENT_TIMEOUT] = std::to_string(unsigned(clientCfg["timeout"]));
    fcConfig.property[FELIX_CLIENT_NETIO_PAGES] = std::to_string(unsigned(clientCfg["netio_pages"]));
    fcConfig.property[FELIX_CLIENT_NETIO_PAGESIZE] = std::to_string(unsigned(clientCfg["netio_pagesize"]));
    fcConfig.property[FELIX_CLIENT_BUS_INTERFACE] = clientCfg["bus_interface"];
    fcConfig.property[FELIX_CLIENT_BUS_GROUP_NAME] = clientCfg["bus_group_name"];
    fcConfig.property[FELIX_CLIENT_BUS_DIR] = clientCfg["bus_dir"];
    fcConfig.property[FELIX_CLIENT_VERBOSE_BUS] = clientCfg["verbose_bus"] ? "True" : "False";
    fcConfig.property[FELIX_CLIENT_VERBOSE_ZYRE] = clientCfg["verbose_zyre"] ? "True" : "False";

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

bool felix_registered = StdDict::registerHwController(
  "FelixClient",
  []() {return std::unique_ptr<HwController>(new FelixController);}
  );
