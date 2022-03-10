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

void FelixRxCore::setRxEnable(uint32_t val) {
  disableRx();

  // TODO: FelixID need be computed from did, cid, elink #, streamId, vid
  FelixID_t fid = val;
  enableChannel(fid);
}

void FelixRxCore::setRxEnable(std::vector<uint32_t> channels) {
  disableRx();

  // TODO: FelixID need be computed from did, cid, elink #, streamId, vid
  std::vector<FelixID_t> fids;
  for (auto chn : channels) {
    fids.push_back(chn);
  }

  enableChannel(fids);
}

void FelixRxCore::disableRx() {
  for (auto& e : m_enables) {
    disableChannel(e.first);
  }
}

void FelixRxCore::maskRxEnable(uint32_t val, uint32_t mask) {
  for(int chan=0; chan<32; chan++) {
    if(!((1<<chan) & mask)) {
      continue;
    }

    if((1<<chan) & val) {
      enableChannel(chan);
    } else {
      disableChannel(chan);
    }
  }
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
  messages_received++;
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

uint32_t FelixRxCore::getCurCount() {return 0;}
bool FelixRxCore::isBridgeEmpty() {return false;}

void FelixRxCore::writeConfig(json &j) {}

void FelixRxCore::loadConfig(const json &j) {
  frlog->info("FelixRxCore:");

  if (j["FelixClient"].contains("flushTime")) {
    m_flushTime = j["FelixClient"]["flushTime"];
    frlog->info(" flush time = {} ms", m_flushTime);
  }
}
