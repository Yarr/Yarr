#include "felix/felix_client_status.h"

#include "FelixRxThread.h"
#include "logging.h"

#include <cstring> // needed for std::memcpy

namespace {
  auto frtlog = logging::make_log("FelixRxThread");
}

FelixRxThread::FelixRxThread(
  std::shared_ptr<SharedClient> client, 
  const std::vector<FelixID_t>& fid_list,
  size_t maxMessageSize
) 
: m_client(client)
, m_maxMessageSize(maxMessageSize)
{
  for (const auto& fid : fid_list) {
    m_fidStats[fid];
  }
}

FelixRxThread::~FelixRxThread() {
  if (thread_ptr and thread_ptr->joinable()) {
    thread_ptr->join();
  }

  // Unsubscribe from all links
  for (const auto& [fid, stats] : m_fidStats) {
    m_client->unsubscribe(fid);
  }

  // Clean up
  // delete data that are not read from rawData
  frtlog->debug("Flush receiver queue...");
  int count = 0;
  while (!m_rawData.empty()) {
    m_rawData.popData();
    count++;
  }
  if (count) {
    frtlog->debug(" ...done ({} stray data blocks)", count);
  } else {
    frtlog->debug(" ...done");
  }
}

void FelixRxThread::run() {
  thread_ptr = std::make_unique<std::thread>(&FelixRxThread::subscribe, this);
}

void FelixRxThread::stop() {
  if (thread_ptr and thread_ptr->joinable()) {
    thread_ptr->join();
  }
}

void FelixRxThread::subscribe() {
  for (auto& [fid, qstat]: m_fidStats) {
    frtlog->debug("Thread 0x{:x} subscribing to fid 0x{:x}", getThreadID(), fid);

    qstat.reset_errors();
    qstat.reset_counters();

    m_client->subscribe(fid, std::bind(&FelixRxThread::on_data_callback, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4), false); // disable all links first
  }
}

void FelixRxThread::on_data_callback(FelixID_t fid, const uint8_t* data, size_t size, uint8_t status) {
  frtlog->trace("Received message from 0x{:x}", fid);

  if (frtlog->should_log(spdlog::level::trace)) {
    frtlog->trace(" message size: {}", size);
    for (size_t b=0; b < size; b++) {
      frtlog->trace(" 0x{:x}", data[b]);
    }
    frtlog->trace(" status: 0x{:x}", status);
  }

  if (m_maxMessageSize > 0 and size > m_maxMessageSize) {
    frtlog->error("dropping the message, because the size is larger than the allowed maximum: {} > {}", size, m_maxMessageSize);
    return;
  }

  // stats
  m_fidStats[fid].messages_received += 1;
  m_fidStats[fid].bytes_received += size;

  // check status
  if (status == FELIX_STATUS_FW_MALF or status == FELIX_STATUS_SW_MALF) {
    m_fidStats[fid].error += 1;
  }
  if (status == FELIX_STATUS_FW_CRC) {
    m_fidStats[fid].crc += 1;
  }
  if (status == FELIX_STATUS_FW_TRUNC or status == FELIX_STATUS_SW_TRUNC) {
    m_fidStats[fid].truncated += 1;
  }

  if (m_doFlushBuffer) return;

  // make RawData from byte array
  uint32_t numWords = (uint32_t)( (size + 3) / 4 );

  if (numWords == 0) return;

  // increment counters before pushData
  m_total_data_in += 1;
  m_total_bytes_in += numWords * sizeof(uint32_t);

  // for now:
  // channel number consists of 6-bit elink, 13-bit link ID, 1-bit is_virtual
  uint32_t mychn = (fid >> 16) & 0x000fffff;

  auto rd = std::make_unique<RawData>(mychn, numWords);

  // copy data to RawData's buffer
  std::memcpy(rd->getBuf(), data, size);

  // push data to the queue
  m_rawData.pushData(std::move(rd));
}

RawDataPtr FelixRxThread::readData() {
  frtlog->debug("FelixRxThread::readData");
  auto rdp = m_rawData.popData();

  if (rdp) {
    m_total_data_out += 1;
    m_total_bytes_out += (rdp->getSize()) * sizeof(uint32_t);
  }

  return rdp;
}

uint32_t FelixRxThread::getDataRate() const {
  double total_byte_rate{0};
  for (const auto& [fid, stats] : m_fidStats) {
    total_byte_rate += stats.byte_rate;
  }

  return total_byte_rate;
}

uint32_t FelixRxThread::getCurCount() const {
  uint64_t cur_cnt = m_total_data_in - m_total_data_out;
  if (cur_cnt > std::numeric_limits<uint32_t>::max()) {
    frtlog->warn("FelixRxThread thread id {}: data counter overflow", getThreadID());
  }
  return cur_cnt;
}

uint32_t FelixRxThread::getCurBytes() const {
  uint64_t cur_byte = m_total_bytes_in - m_total_bytes_out;
  if (cur_byte > std::numeric_limits<uint32_t>::max()) {
    frtlog->warn("FelixRxThread thread id {}: byte counter overflow", getThreadID());
  }
  return cur_byte;
}

void FelixRxThread::resetStatistics() {
  for (auto& [fid, stats] : m_fidStats) {
    stats.reset_counters();
  }
}

void FelixRxThread::computeRates(const double& time /* seconds */) {
  for (auto& [fid, stats] : m_fidStats) {
    stats.msg_rate = stats.messages_received / time; // Hz
    stats.byte_rate = stats.bytes_received / time; // B/s
  }
}

void FelixRxThread::reportStatistics() {
  for (const auto& [fid, stats] : m_fidStats) {
    frtlog->info("Rx fid 0x{:x}: data rate = {:.2f} Mb/s  message rate = {:.2f} kHz", fid, stats.byte_rate*8e-6, stats.msg_rate/1000);

    if (stats.error or stats.crc or stats.truncated) {
      frtlog->warn("FELIX errors on fid 0x{:x}: fw/sw errors = {}  crc errors = {}  fw/sw truncations = {}", fid, stats.error, stats.crc, stats.truncated);
    }
  }

  frtlog->debug("Thread {}: data size in rx queue: {} MB (in: {} MB, out: {} MB)", getThreadID(), (m_total_bytes_in - m_total_bytes_out)/1e6, m_total_bytes_in/1e6, m_total_bytes_out/1e6);
}