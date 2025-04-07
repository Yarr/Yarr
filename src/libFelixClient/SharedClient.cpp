#include "SharedClient.h"
#include "logging.h"

namespace {
  auto scllog = logging::make_log("SharedClient");
  auto sctimer = logging::make_log("SharedClientTimer");
}

SharedClient::SharedClient(const json &cfg) {
  FelixClientThread::Config fcConfig;
  fcConfig.property[FELIX_CLIENT_LOCAL_IP_OR_INTERFACE] = cfg["localIPorInterface"];
  fcConfig.property[FELIX_CLIENT_LOG_LEVEL] = cfg["logLevel"];
  fcConfig.property[FELIX_CLIENT_BUS_DIR] = cfg["busDir"];
  fcConfig.property[FELIX_CLIENT_BUS_GROUP_NAME] = cfg["busGroupName"];
  fcConfig.property[FELIX_CLIENT_VERBOSE_BUS] = cfg["verboseBus"] ? "True" : "False";
  fcConfig.property[FELIX_CLIENT_TIMEOUT] = std::to_string(unsigned(cfg["timeout"]));
  fcConfig.property[FELIX_CLIENT_NETIO_PAGES] = std::to_string(unsigned(cfg["netioPages"]));
  fcConfig.property[FELIX_CLIENT_NETIO_PAGESIZE] = std::to_string(unsigned(cfg["netioPagesize"]));

  fcConfig.on_init_callback = std::bind(&SharedClient::on_init, this);
  fcConfig.on_data_callback = std::bind(&SharedClient::on_data_received, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4);
  fcConfig.on_connect_callback = std::bind(&SharedClient::on_connect, this, std::placeholders::_1);
  fcConfig.on_disconnect_callback = std::bind(&SharedClient::on_disconnect, this, std::placeholders::_1);

  m_client = std::make_unique<FelixClientThread>(fcConfig);
}

SharedClient::~SharedClient() = default;

void SharedClient::subscribe(FelixID_t fid, const DataCallback& callback) {
  {
    std::unique_lock lock(mtx);
    // register the callback
    m_callbacks[fid] = callback;
  }
  m_client->subscribe(fid);
}

void SharedClient::resubscribe(FelixID_t fid, bool enable) {
  m_client->subscribe(fid);
}

void SharedClient::unsubscribe(FelixID_t fid) {
  m_client->unsubscribe(fid);
}

void SharedClient::enableTx(FelixID_t fid) {
  std::unique_lock lock(mtx);
  m_txEnables[fid] = true;
  scllog->trace("Enable Tx link 0x{:x}", fid);
}

void SharedClient::enableTx() {
  std::unique_lock lock(mtx);
  for (auto& [fid, enable] : m_txEnables) {
    m_txEnables[fid] = true;
    scllog->trace("Enable Tx link 0x{:x}", fid);
  }
}

void SharedClient::disableTx(FelixID_t fid) {
  std::unique_lock lock(mtx);
  m_txEnables[fid] = false;
  scllog->trace("Disable Tx link 0x{:x}", fid);
}

void SharedClient::disableTx() {
  std::unique_lock lock(mtx);
  for (auto& [fid, enable] : m_txEnables) {
    m_txEnables[fid] = false;
    scllog->trace("Disable Tx link 0x{:x}", fid);
  }
}

void SharedClient::enableRx(FelixID_t fid) {
  std::unique_lock lock(mtx);
  m_rxEnables[fid] = true;
  scllog->trace("Enable Rx link 0x{:x}", fid);
}

void SharedClient::enableRx() {
  std::unique_lock lock(mtx);
  for (auto& [fid, enable] : m_rxEnables) {
    m_rxEnables[fid] = true;
    scllog->trace("Enable Rx link 0x{:x}", fid);
  }
}

void SharedClient::disableRx(FelixID_t fid) {
  std::unique_lock lock(mtx);
  m_rxEnables[fid] = false;
  scllog->trace("Disable Rx link 0x{:x}", fid);
}

void SharedClient::disableRx() {
  std::unique_lock lock(mtx);
  for (auto& [fid, enable] : m_rxEnables) {
    m_rxEnables[fid] = false;
    scllog->trace("Disable Rx link 0x{:x}", fid);
  }
}

bool SharedClient::isTxEnabled(FelixID_t fid) {
  std::shared_lock lock(mtx);
  return m_txEnables[fid];
}

bool SharedClient::isRxEnabled(FelixID_t fid) {
  std::shared_lock lock(mtx);
  return m_rxEnables[fid];
}

void SharedClient::on_connect(FelixID_t fid) {
  scllog->debug("Connect to FELIX link 0x{:x}", fid);
}

void SharedClient::on_disconnect(FelixID_t fid) {
  scllog->debug("Disconnect from FELIX link 0x{:x}", fid);
}

void SharedClient::on_data_received(FelixID_t fid, const uint8_t* data, size_t size, uint8_t status) {
  // skip if the channel is disabled
  if (not m_rxEnables[fid]) return;

  sctimer->trace("SharedClient::on_data_received start fid=0x{:x} size={}", fid, size);

  #define UNLIKELY(x) __builtin_expect(x,0)
  if (m_last_id != fid) {
    m_last_it = m_callbacks.find(fid);
    if (UNLIKELY(m_last_it == m_callbacks.end())) {
      scllog->warn("No callback was found for FELIX link 0x{:x}", fid);
      m_last_id = -1;
      return;
    }
    m_last_id = fid;
  }
  m_last_it->second(fid, data, size, status);
  sctimer->trace("SharedClient::on_data_received done fid=0x{:x}", fid);
}