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
    fcConfig.on_init_callback = std::bind(&FelixController::on_init, this);
    fcConfig.on_data_callback = std::bind(&FelixController::on_data, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4);
    fcConfig.on_connect_callback = std::bind(&FelixController::on_connect, this, std::placeholders::_1);
    fcConfig.on_disconnect_callback = std::bind(&FelixController::on_disconnect, this, std::placeholders::_1);

    // Properties
    // See https://gitlab.cern.ch/atlas-tdaq-felix/felix-interface/-/blob/4.2.x/felix/felix_client_properties.h
    fcConfig.property[FELIX_CLIENT_LOCAL_IP_OR_INTERFACE] = std::string(j["FelixClient"]["local_ip_or_interface"]);
    fcConfig.property[FELIX_CLIENT_LOG_LEVEL] = std::string(j["FelixClient"]["log_level"]);
    fcConfig.property[FELIX_CLIENT_TIMEOUT] = std::to_string(unsigned(j["FelixClient"]["timeout"]));
    fcConfig.property[FELIX_CLIENT_NETIO_PAGES] = std::to_string(unsigned(j["FelixClient"]["netio_pages"]));
    fcConfig.property[FELIX_CLIENT_NETIO_PAGESIZE] = std::to_string(unsigned(j["FelixClient"]["netio_pagesize"]));
    fcConfig.property[FELIX_CLIENT_BUS_INTERFACE] = std::string(j["FelixClient"]["bus_interface"]);
    fcConfig.property[FELIX_CLIENT_BUS_GROUP_NAME] = std::string(j["FelixClient"]["bus_group_name"]);
    fcConfig.property[FELIX_CLIENT_BUS_DIR] = std::string(j["FelixClient"]["bus_dir"]);
    fcConfig.property[FELIX_CLIENT_VERBOSE_BUS] = j["FelixClient"]["verbose_bus"] ? "True" : "False";
    fcConfig.property[FELIX_CLIENT_VERBOSE_ZYRE] = j["FelixClient"]["verbose_zyre"] ? "True" : "False";

    // Construct felix client
    client = std::make_unique<FelixClientThread>(fcConfig);

  } catch (std::runtime_error &fce) {
    fclog->error("Failed to construct Felix client");
    throw fce;
  } 

  try {
    FelixTxCore::loadConfig(j);
  } catch (std::runtime_error &je) {
    fclog->error("Failed to load FelixTxCore config");
    throw je;
  }

  try {
    FelixRxCore::loadConfig(j);
  } catch (std::runtime_error &je) {
    fclog->error("Failed to load FelixRxCore config");
    throw je;
  }
}

bool felix_registered = StdDict::registerHwController(
  "FelixClient",
  []() {return std::unique_ptr<HwController>(new FelixController);}
  );
