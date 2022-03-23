#include "FelixRxCore.h"

#include "logging.h"

#include <cstring> // needed for std::memcpy

namespace {
  auto frlog = logging::make_log("FelixRxCore");
}

void FelixRxCore::enableChannel(FelixID_t fid) {
  frlog->debug("Subscribe to Rx link: 0x{:x}", fid);
  m_enables[fid] = true;
  fclient->subscribe(fid);
  m_t0 = std::chrono::steady_clock::now();
}

void FelixRxCore::enableChannel(std::vector<FelixID_t> fids) {
  for (const auto& fid : fids) {
    frlog->debug("Subscribe to Rx link: 0x{:x}", fid);
    m_enables[fid] = true;
  }
  fclient->subscribe(fids);
  m_t0 = std::chrono::steady_clock::now();
}

void FelixRxCore::disableChannel(FelixID_t fid) {
  frlog->debug("Unsubscribe from Rx link: 0x{:x}", fid);
  m_enables[fid] = false;
  fclient->unsubscribe(fid);
}

FelixRxCore::FelixID_t FelixRxCore::fid_from_channel(uint32_t chn) {
  // Compute FelixID from did, cid, link id elink #, streamId

  // TODO: get link/GBT id and elink # from chn?
  uint16_t link_id = 0; // FIXME
  uint8_t elink = chn;

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

  std::vector<FelixID_t> fids;
  for (auto chn : channels) {
    auto fid = fid_from_channel(chn);
    fids.push_back(fid);
  }

  enableChannel(fids);
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
  std::this_thread::sleep_for(std::chrono::milliseconds(m_flushTime));
  m_doFlushBuffer = false;
}

RawData* FelixRxCore::readData() {
  frlog->debug("FelixRxCore::readData");
  std::unique_ptr<RawData> rdp = rawData.popData();

  if (rdp)
    ++total_data_out;

  return rdp.release();
}

void FelixRxCore::on_data(uint64_t fid, const uint8_t* data, size_t size, uint8_t status) {
  frlog->debug("Received message from 0x{:x}", fid);

  frlog->trace(" message size: {}", size);
  for (size_t b=0; b < size; b++) {
    frlog->trace(" 0x{:x}", data[b]);
  }
  frlog->trace(" status: 0x{:x}", status);

  // stats
  ++messages_received;
  bytes_received += size;

  if (m_doFlushBuffer)
    return;

  // make RawData from byte array
  uint32_t numWords = (uint32_t)( (size + 3) / 4 );

  if (numWords == 0)
    return;

  std::unique_ptr<uint32_t[]> buffer(new uint32_t[numWords]);
  std::memcpy(buffer.get(), data, size);

  // for now
  uint32_t mychn = fid & 0xffffffff;

  rawData.pushData(std::make_unique<RawData>(mychn, buffer.release(), numWords));

  ++total_data_in;
}

void FelixRxCore::setClient(FelixClientThread* client) {
  fclient = client;
}

void FelixRxCore::checkDataRate() {
  // reset counters and start time
  messages_received = 0;
  bytes_received = 0;
  m_t0 = std::chrono::steady_clock::now();

  // wait for one second
  std::this_thread::sleep_for(std::chrono::milliseconds(1000));

  std::chrono::duration<double> time = std::chrono::steady_clock::now() - m_t0;

  byte_rate = bytes_received / time.count(); // Byte/s
  msg_rate = messages_received / time.count(); // Hz

  frlog->info("Data rate: {:.2f} Mb/s  message rate: {:.2f} kHz", byte_rate*8e-6, msg_rate/1000.);
}

uint32_t FelixRxCore::getDataRate() {
  if (byte_rate < 0) {
    // byte_rate has never been calculated
    // estimate it here
    std::chrono::duration<double> seconds = std::chrono::steady_clock::now() - m_t0;
    return bytes_received / seconds.count();
  } else {
    // byte_rate has been computed in checkDataRate()
    return byte_rate;
  }
}

uint32_t FelixRxCore::getCurCount() {
  uint64_t cur_cnt = total_data_in - total_data_out;
  if (cur_cnt > 0xffffffff) {
    frlog->warn("FelixRxCore: counter overflow");
  }
  return cur_cnt;
}

// WIP
FelixRxCore::FelixRxCore()
{

}

FelixRxCore::~FelixRxCore()
{
  // Unsubscribe from all links
  disableRx();

  // Clean up
  // delete data that are not read from rawData
  frlog->debug("Flush receiver queue...");
  int count = 0;
  while (!rawData.empty()) {
    auto data = rawData.popData();
    count++;
    delete [] data->buf;
  }
  if (count) {
    frlog->debug(" ...done ({} stray data blocks)", count);
  } else {
    frlog->debug(" ...done");
  }
}

bool FelixRxCore::isBridgeEmpty() {return false;}

void FelixRxCore::writeConfig(json &j) {}

void FelixRxCore::loadConfig(const json &j) {
  frlog->info("FelixRxCore:");

  if (j.contains("flushTime")) {
    m_flushTime = j["flushTime"];
    frlog->info(" flush time = {} ms", m_flushTime);
  }

  if (j.contains("detector_id")) {
    m_did = j["detector_id"];
    frlog->info(" did = {}", m_did);
  }
  if (j.contains("connector_id")) {
    m_cid = j["connector_id"];
    frlog->info(" cid = {}", m_cid);
  }
  if (j.contains("protocol")) {
    m_protocol = j["protocol"];
    frlog->info(" protocol = {}", m_protocol);
  }
}
