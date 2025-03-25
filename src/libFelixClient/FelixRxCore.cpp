#include "FelixRxCore.h"
#include "logging.h"

namespace {
  auto frlog = logging::make_log("FelixRxCore");
}

FelixRxCore::FelixRxCore() = default;

FelixRxCore::~FelixRxCore()
{
  for (auto& frt : m_rxThreads) {
    frt->stop();
  }

  stopMonitor();
}

void FelixRxCore::initRxChannels(const std::vector<uint32_t>& channels) {
  frlog->info("Initializing Rx channels");
  std::vector<std::vector<FelixID_t>> fid_lists(m_nThreads);

  unsigned ithread {0};
  for (auto chn : channels) {
    auto fid = fid_from_channel(chn);
    fid_lists[ithread%m_nThreads].push_back(fid);
    ithread++;
  }

  // Start threads to subscribe to channels
  for (unsigned i=0; i<m_nThreads; i++) {
    m_rxThreads.emplace_back(std::make_unique<FelixRxThread>(m_client, fid_lists[i]));
    m_rxThreads.back()->run();
  }

  if (m_runMonitor) {
    runMonitor();
  }
}

void FelixRxCore::enableChannel(FelixID_t fid) {
  frlog->debug("Enable Rx link: 0x{:x}", fid);
  m_client->enableRx(fid);
}

void FelixRxCore::disableChannel(FelixID_t fid) {
  frlog->debug("Disable Rx link: 0x{:x}", fid);
  m_client->disableRx(fid);
}

FelixRxCore::FelixID_t FelixRxCore::fid_from_channel(uint32_t chn) {
  // Compute FelixID from did, cid, link id elink #, streamId

  // Get link/GBT id and elink # from channel number
  uint8_t elink = FelixTools::elink_from_chn(chn);
  uint16_t link_id = FelixTools::link_from_chn(chn);

  // Hard code is_virtual to false, and streamID to 0 for now
  bool is_virtual = false;
  uint8_t sid = 0;

  return FelixTools::get_fid(
    m_did, m_cid, is_virtual, link_id, elink, false, m_protocol, sid
    );
}

void FelixRxCore::setRxEnable(uint32_t val) {
  disableRx();

  auto fid = fid_from_channel(val);
  enableChannel(fid);
}

void FelixRxCore::setRxEnable(std::vector<uint32_t> channels) {
  disableRx();

  for (auto chn : channels) {
    auto fid = fid_from_channel(chn);
    enableChannel(fid);
  }
}

void FelixRxCore::disableRx() {
  m_client->disableRx();
}

// still needed?
void FelixRxCore::maskRxEnable(uint32_t val, uint32_t mask) {
  frlog->warn("FelixRxCore::maskRxEnable is not implemented");
}

void FelixRxCore::flushBuffer() {
  // Flush the receiver queue
  for (auto& frt : m_rxThreads) frt->flush(true);
  std::this_thread::sleep_for(std::chrono::milliseconds(m_flushWaitTime));
  for (auto& frt : m_rxThreads) frt->flush(false);
}

std::vector<RawDataPtr> FelixRxCore::readData() {
  frlog->trace("FelixRxCore::readData");
  std::vector<RawDataPtr> dataVec;

  for (auto& frt : m_rxThreads) {
    auto data = frt->readData();
    if (data) {
      dataVec.push_back(std::move(data));
    }
  }

  return dataVec;
}

void FelixRxCore::setClient(std::shared_ptr<SharedClient> client) {
  m_client = client;
}

uint32_t FelixRxCore::getDataRate() {
  uint32_t data_rate{0};
  for (auto& frt : m_rxThreads) {
    data_rate += frt->getDataRate();
  }
  return data_rate;
}

uint32_t FelixRxCore::getCurCount() {
  uint32_t cur_cnt{0};
  for (auto& frt : m_rxThreads) {
    cur_cnt += frt->getCurCount();
  }
  return cur_cnt;
}

bool FelixRxCore::isBridgeEmpty() {return false;}

void FelixRxCore::loadConfig(const json &j) {
  frlog->info("FelixRxCore:");

  if (j.contains("flushWaitTime")) {
    m_flushWaitTime = j["flushWaitTime"];
    frlog->info(" flush wait time = {} ms", m_flushWaitTime);
  }

  if (j.contains("detectorID")) {
    m_did = j["detectorID"];
    frlog->info(" did = {}", m_did);
  }
  if (j.contains("connectorID")) {
    m_cid = j["connectorID"];
    frlog->info(" cid = {}", m_cid);
  }
  if (j.contains("protocol")) {
    m_protocol = j["protocol"];
    frlog->info(" protocol = {}", m_protocol);
  }

  if (j.contains("enableMonitor")) {
    m_runMonitor = j["enableMonitor"];
    frlog->info(" run monitor = {}", m_runMonitor);
  }
  if (j.contains("monitorInterval")) {
    m_interval_ms = j["monitorInterval"];
    frlog->info(" monitor interval = {} ms", m_interval_ms);
  }
  if (j.contains("queueLimitMB")) {
    m_queue_limit = j["queueLimitMB"];
    frlog->info(" queue limit = {} MB", m_queue_limit);
  }
  if (j.contains("maxMessageSize")) {
    m_maxMessageSize = j["maxMessageSize"];
    if (m_maxMessageSize>0) frlog->info(" message size limit = {} B", m_maxMessageSize);
    else frlog->info(" message size limit = unlimited");
  }

  if (j.contains("nthreads")) {
    m_nThreads = j["nthreads"];
    frlog->info(" nthreads = {}", m_nThreads);
  }
}

void FelixRxCore::writeConfig(json &j) {
  j["detectorID"] = m_did;
  j["connectorID"] = m_cid;
  j["protocol"] = m_protocol;
  j["flushWaitTime"] = m_flushWaitTime;
  j["enableMonitor"] = m_runMonitor;
  j["monitorInterval"] = m_interval_ms;
  j["queueLimitMB"] = m_queue_limit;
  j["nthreads"] = m_nThreads;
}

void FelixRxCore::runMonitor(bool print_info) {
  for (auto& frt : m_rxThreads) {
    frt->runMonitor(m_interval_ms, m_queue_limit, print_info);
  }
}

void FelixRxCore::stopMonitor() {
  for (auto& frt : m_rxThreads) {
    frt->stopMonitor();
  }
}