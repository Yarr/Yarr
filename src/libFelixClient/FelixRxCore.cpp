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
    if (fid_lists[i].empty()) {
      // skip in case there are more threads than fids
      continue;
    }

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