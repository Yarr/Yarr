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

  // Unsubscribe from all links
  for (const auto& [fid, stats] : m_qStats) {
    if (stats.connected) {
      fclient->unsubscribe(fid);
    }
  }

  // Clean up
  // delete data that are not read from rawData
  frlog->debug("Flush receiver queue...");
  int count = 0;
  while (!m_rawData.empty()) {
    m_rawData.popData();
    count++;
  }

  if (m_runMonitor) {
    runMonitor();
  }
}

void FelixRxCore::enableChannel(FelixID_t fid) {
  frlog->debug("Subscribe to Rx link: 0x{:x}", fid);
  try {
    fclient->subscribe(fid);
    m_enables[fid] = true;
    m_qStats[fid];
  } catch (std::runtime_error& e) {
    frlog->warn("Fail to subscribe to Rx link 0x{:x}: {}", fid, e.what());
  }
}

void FelixRxCore::disableChannel(FelixID_t fid) {
  frlog->debug("Unsubscribe from Rx link: 0x{:x}", fid);
  if (m_enables.find(fid) != m_enables.end()) {
    m_enables[fid] = false;
  } else {
    frlog->warn("Rx link 0x{:x} was never enabled", fid);
  }
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
  for (auto& e : m_enables) {
    disableChannel(e.first);
  }
}

// still needed?
void FelixRxCore::maskRxEnable(uint32_t val, uint32_t mask) {
  frlog->warn("FelixRxCore::maskRxEnable is not implemented");
}

void FelixRxCore::flushBuffer() {
  // Flush the receiver queue
  m_doFlushBuffer = true;
  std::this_thread::sleep_for(std::chrono::milliseconds(m_flushWaitTime));
  m_doFlushBuffer = false;
}

std::vector<RawDataPtr> FelixRxCore::readData() {
  frlog->trace("FelixRxCore::readData");
  std::vector<RawDataPtr> dataVec;

  std::unique_ptr<RawData> rdp = m_rawData.popData();

  if (rdp) {
    m_total_data_out += 1;
    m_total_bytes_out += (rdp->getSize()) * sizeof(uint32_t);

    dataVec.push_back(std::move(rdp));
  }

  return dataVec;
}

void FelixRxCore::on_data(FelixID_t fid, const uint8_t* data, size_t size, uint8_t status) {
  // skip if the channel is disabled
  if (not m_enables[fid])
    return;

  frlog->trace("Received message from 0x{:x}", fid);

  if (frlog->should_log(spdlog::level::trace)){
    frlog->trace(" message size: {}", size);
    for (size_t b=0; b < size; b++) {
      frlog->trace(" 0x{:x}", data[b]);
    }
  }

  // stats
  m_qStats[fid].messages_received += 1;
  m_qStats[fid].bytes_received += size;

  if (status == FELIX_STATUS_FW_MALF or status == FELIX_STATUS_SW_MALF) {
    m_qStats[fid].error += 1;
  }
  if (status == FELIX_STATUS_FW_CRC) {
    m_qStats[fid].crc += 1;
  }
  if (status == FELIX_STATUS_FW_TRUNC or status == FELIX_STATUS_SW_TRUNC) {
    m_qStats[fid].truncated += 1;
  }

  if (m_doFlushBuffer)
    return;

  // make RawData from byte array
  uint32_t numWords = (uint32_t)( (size + 3) / 4 );

  if (numWords == 0)
    return;

  // increment counters before pushData
  m_total_data_in += 1;
  m_total_bytes_in += numWords * sizeof(uint32_t);

  // for now:
  // channel number consists of 6-bit elink, 13-bit link ID, 1-bit is_virtual
  uint32_t mychn = (fid >> 16) & 0x000fffff;

  auto rd = std::make_unique<RawData>(mychn, numWords);

  // copy data to RawData's buffer
  std::memcpy(rd->getBuf(), data, size);

  m_rawData.pushData(std::move(rd));
}

void FelixRxCore::on_connect(FelixID_t fid) {
  try {
    m_qStats.at(fid).connected = true;
  } catch (std::out_of_range &e) {
    frlog->trace("Stats of fid 0x{:x} is not tracked.");
  }
}

void FelixRxCore::on_disconnect(FelixID_t fid) {
  try {
    m_qStats.at(fid).connected = false;
  } catch (std::out_of_range &e) {
    // For example, this is an fid used for reading/writing FELIX registers
    frlog->trace("Stats of fid 0x{:x} is not tracked.");
  }
}

void FelixRxCore::setClient(std::shared_ptr<FelixClientThread> client) {
  fclient = client;
}

uint32_t FelixRxCore::getDataRate() {
  double total_byte_rate{0};
  for (const auto& [fid, stats] : m_qStats) {
    total_byte_rate += stats.byte_rate;
  }

  if (total_byte_rate < 0) {
    // Monitor is not run
    frlog->warn("Data rates have not been calculated. Call FelixRxCore::runMonitor to check the Rx queue.");
    return 0;
  }
  return data_rate;
}

uint32_t FelixRxCore::getCurCount() {
  uint64_t cur_cnt = m_total_data_in - m_total_data_out;
  if (cur_cnt > std::numeric_limits<uint32_t>::max()) {
    frlog->warn("FelixRxCore: counter overflow");
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

  if (j.contains("waitTime")) {
    m_waitTime = std::chrono::microseconds(j["waitTime"]);
    frlog->info(" rx wait time = {} microseconds", m_waitTime.count());
  }

  if (m_runMonitor) {
    runMonitor();
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