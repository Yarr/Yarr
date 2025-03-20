#include "felix/felix_client_status.h"

#include "FelixRxThread.h"
#include "logging.h"

namespace {
  auto frtlog = logging::make_log("FelixRxThread");
}

FelixRxThread::FelixRxThread(
  std::shared_ptr<SharedClient> client, 
  const std::vector<FelixID_t>& fid_list,
  ClipBoard<RawData>* data_buffer
) 
: m_client(client)
, m_rawData(data_buffer)
{
  for (const auto& fid : fid_list) {
    m_fidStats[fid];
  }
}

FelixRxThread::~FelixRxThread() {
  if (thread_ptr and thread_ptr->joinable()) {
    thread_ptr->join();
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
    std::stringstream ss;
    ss << std::this_thread::get_id();
    frtlog->trace("Thread {} subscribing to fid 0x{:x}", ss.str(), fid);

    qstat.reset_errors();
    qstat.reset_counters();

    m_client->subscribe(fid, std::bind(&FelixRxThread::on_data_callback, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4));
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
  m_rawData->pushData(std::move(rd));
}