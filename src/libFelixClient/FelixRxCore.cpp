#include "FelixRxCore.h"
#include "logging.h"

namespace {
  auto frlog = logging::make_log("FelixRxCore");
}

FelixRxCore::FelixRxCore() = default;

FelixRxCore::~FelixRxCore()
{
  stopMonitor();
}

void FelixRxCore::initRxChannels(const std::vector<uint32_t>& channels) {
  frlog->info("Initializing Rx channels");

  if (m_nThreads > channels.size()) {
    frlog->warn("The number of requested threads ({}) is larger than the number of Rx channels ({}). Only {} threads will be created.", m_nThreads, channels.size(), channels.size());
  }

  std::vector<std::vector<FelixID_t>> fid_lists(m_nThreads);

  unsigned ithread {0};
  for (auto chn : channels) {
    auto fid = fid_from_channel(chn);
    fid_lists[ithread%m_nThreads].push_back(fid);
    m_fidThreadMap[fid] = ithread%m_nThreads;
    ithread++;
  }

  // Start threads to subscribe to channels
  for (unsigned i=0; i<m_nThreads; i++) {
    // skip in case there are more threads than fids
    if (fid_lists[i].empty()) continue;

    m_rxThreads.emplace_back(std::make_unique<FelixRxThread>(m_fcConfig, fid_lists[i], m_maxMessageSize));
  }

  for (auto& frt : m_rxThreads) {
    frt->run();
  }

  // Wait all fids to be connected
  while (true) {
    bool all_connected = true;
    for (auto& frt : m_rxThreads) {
      if (!frt->allConnected()) {
        all_connected = false;
        break;
      }
    }
    if (all_connected) break;
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
  }
  frlog->debug("All channels connected");

  if (m_runMonitor) {
    runMonitor();
  }
}

void FelixRxCore::enableChannel(FelixID_t fid) {
  frlog->debug("Enable Rx link: 0x{:x}", fid);
  try {
    m_rxThreads[m_fidThreadMap[fid]]->enableChannel(fid);
  } catch (const std::out_of_range& e) {
    frlog->error("Failed to enable channel: unknown FelixID 0x{:x}", fid);
  }
}

void FelixRxCore::disableChannel(FelixID_t fid) {
  frlog->debug("Disable Rx link: 0x{:x}", fid);
  try {
    m_rxThreads[m_fidThreadMap[fid]]->disableChannel(fid);
  } catch (const std::out_of_range& e) {
    frlog->error("Failed to disable channel: unknown FelixID 0x{:x}", fid);
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

FelixRxCore::FelixID_t FelixRxCore::ic_fid_from_channel(uint32_t chn) {
  // Compute FelixID from did, cid, channel number
  // for IC in the rx direction (to-host), the designated egroup is 6 and epath is 1 for IC communcication
  // The link_id is the same as for non-ic communication but the elink is offset by 25
  uint16_t link_id = FelixTools::link_from_chn(chn);
  uint8_t elink = 25;
  bool is_virtual = false;
  uint8_t sid = 0;

  return FelixTools::get_fid(
    m_did, m_cid, is_virtual, link_id, elink, false, m_protocol, sid
    );
}

void FelixRxCore::setRxEnable(std::vector<uint32_t> channels) {
  disableRx();

  for (auto chn : channels) {
    auto fid = fid_from_channel(chn);
    enableChannel(fid);
  }
}

void FelixRxCore::disableRx() {
  for (auto& frt : m_rxThreads) {
    frt->disableChannel();
  }
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

void FelixRxCore::clearRawData(){
  // Clear out the raw data stored in m_rawData
  frlog->debug("Emptying out the raw data buffer");
  // Should we disable all channels first?
  for (auto& frt : m_rxThreads) {
    frt->clearRawData();
  }
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

uint32_t FelixRxCore::getDataRate() {
  double data_rate{0};
  for (auto& frt : m_rxThreads) {
    data_rate += frt->getDataRate();
  }

  if (data_rate <= 0) {
    // Monitor is not run
    frlog->warn("Data rates have not been calculated. Call FelixRxCore::runMonitor to check the Rx queue.");
    return 0;
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
    frlog->info(" run monitor = {}", m_runMonitor.load());
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

  if (j.contains("nthreads")) {
    m_nThreads = j["nthreads"];
    frlog->info(" nthreads = {}", m_nThreads);
  }
}

void FelixRxCore::setClient(const FelixClientThread::Config& fcConfig) {
  m_fcConfig = fcConfig;
}

void FelixRxCore::writeConfig(json &j) {
  j["detectorID"] = m_did;
  j["connectorID"] = m_cid;
  j["protocol"] = m_protocol;
  j["flushWaitTime"] = m_flushWaitTime;
  j["enableMonitor"] = m_runMonitor.load();
  j["monitorInterval"] = m_interval_ms;
  j["queueLimitMB"] = m_queue_limit;
  j["nthreads"] = m_nThreads;
}

void FelixRxCore::runMonitor(bool print_info) {
  // stop the monitoring loop in case it has been running
  stopMonitor();

  frlog->debug("Starting monitor thread");
  m_runMonitor = true;

  m_monitor_thread = std::thread([this, print_info]{
    if (frlog->should_log(spdlog::level::trace)) {
      std::stringstream ss;
      ss << "0x" << std::hex << std::this_thread::get_id();
      frlog->trace("Monitor thread id {}", ss.str());
    }

    while (m_runMonitor) {
      // Check data size in each Rx queue
      for (auto& frt : m_rxThreads) {
        if (frt->getCurBytes() > m_queue_limit*1e6) {
          // Too much data to handle. Stop adding data before OOM
          frlog->critical("Rx thread {}: data are not consumed quickly enough!! Stop taking data into Rx queue ...", frt->getThreadID());
          frt->flush(true);
        }
      }

      // Data rate
      m_t0 = std::chrono::steady_clock::now();
      for (auto& frt : m_rxThreads) {
        frt->resetStatistics();
      }

      // wait
      std::this_thread::sleep_for(std::chrono::milliseconds(m_interval_ms));

      std::chrono::duration<double> time = std::chrono::steady_clock::now() - m_t0;
      for (auto& frt : m_rxThreads) {
        frt->computeRates(time.count());
      }

      if (print_info) {
        frlog->info("--------------------------------");
        for (auto& frt : m_rxThreads) {
          frt->reportStatistics();
        }
      }

    } // end of while (m_runMonitor)

    frlog->debug("Rx monitor finished");
  });
}

void FelixRxCore::stopMonitor() {
  m_runMonitor = false;
  if (m_monitor_thread.joinable()) m_monitor_thread.join();
}