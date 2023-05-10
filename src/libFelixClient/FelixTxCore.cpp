#include "FelixTxCore.h"

#include "logging.h"

#include <sstream>
#include <iomanip>

namespace {
  auto ftlog = logging::make_log("FelixTxCore");
}

FelixTxCore::FelixTxCore() = default;

FelixTxCore::~FelixTxCore()
{
  if (m_trigProc.joinable()) m_trigProc.join();
}

// Channel control
void FelixTxCore::enableChannel(FelixID_t fid) {
  ftlog->debug("Enable Tx link: 0x{:x}", fid);
  if (checkChannel(fid)) {
    m_enables[fid] = true;
    m_fifo[fid];
  }
}

void FelixTxCore::disableChannel(FelixID_t fid) {
  ftlog->debug("Disable Tx link: 0x{:x}", fid);
  m_enables[fid] = false;
}

FelixTxCore::FelixID_t FelixTxCore::fid_from_channel(uint32_t chn) {
  // Compute FelixID from did, cid, link id elink #, streamId

  // Get link/GBT id and elink # from channel number
  // chn[18:6] is the link ID; chn[5:0] is the e-link number
  uint8_t elink = chn & 0x3f;
  uint16_t link_id = (chn >> 6) & 0x1fff;

  // Hard code is_virtual to false, and streamID to 0 for now
  bool is_virtual = false;
  uint8_t sid = 0;

  return FelixTools::get_fid(
    m_did, m_cid, is_virtual, link_id, elink, true, m_protocol, sid
    );
}

bool FelixTxCore::checkChannel(FelixID_t fid) {
  ftlog->debug("Try sending data to Tx link: 0x{:x}",fid);
  try {
    std::string empty;
    fclient->send_data(fid, (const uint8_t*)empty.c_str(), 1, true);
  } catch (std::runtime_error& e) {
    ftlog->warn("Fail to send to Tx link 0x{:x}: {}", fid, e.what());
    return false;
  }

  return true;
}

void FelixTxCore::prepareBroadcast() {
  auto fid_broadcast = fid_from_channel(BroadcastChn);

  // Check if the broadcast fid is available
  if (not checkChannel(fid_broadcast)) {
    ftlog->error("Broadcast link not available.");
    ftlog->info("At least one of the FELIX broadcast registers (BROADCAST_ENABLE_XX) need to be set to non-zero values before starting the FELIX-Star processes.");
    throw std::runtime_error("Fail to broadcast");
  }

  // Initialize the fifo
  m_fifo[fid_broadcast];

  // Set FELIX BROADCAST_ENABLE_[00:23] based on m_enables
  ftlog->info("Set FELIX broadcast enable registers");
  // Max 24 link per FELIX device
  std::array<uint64_t, 24> broadcastRegValues;
  for (size_t l=0; l<24; l++) broadcastRegValues[l] = 0;

  for (const auto& [fid, enable] : m_enables) {
    auto link_id = FelixTools::link_from_fid(fid);
    assert(link_id < 24);

    auto elink = FelixTools::elink_from_fid(fid);

    if (enable) {
      broadcastRegValues[link_id] |= (1 << elink);
    }
  }

  // Write the broadcast registers
  for (size_t l=0; l<24; l++) {
    std::stringstream brdcstRegName;
    brdcstRegName << "BROADCAST_ENABLE_" << std::setfill('0') << std::setw(2) << l;

    writeFelixRegister(brdcstRegName.str(), std::to_string(broadcastRegValues[l]));
  }
}

void FelixTxCore::setCmdEnable(uint32_t chn) {
  // Switch off all channels first
  disableCmd();

  auto fid = fid_from_channel(chn);
  enableChannel(fid);

  if (m_broadcast) {
    try {
      prepareBroadcast();
    } catch (std::runtime_error &e) {
      ftlog->critical(e.what());
    }
  }
}

void FelixTxCore::setCmdEnable(std::vector<uint32_t> chns) {
  // Switch off all channels first
  disableCmd();

  for (auto& c : chns) {
    auto fid = fid_from_channel(c);
    enableChannel(fid);
  }

  if (m_broadcast) {
    try {
      prepareBroadcast();
    } catch (std::runtime_error &e) {
      ftlog->critical(e.what());
    }
  }
}

void FelixTxCore::disableCmd() {
  for (auto& e : m_enables) {
    disableChannel(e.first);
  }
}

uint32_t FelixTxCore::getCmdEnable() { // unused
  return 0;
}

bool FelixTxCore::isCmdEmpty() {
  for (const auto& [chn, buffer] : m_fifo) {
    // consider only enabled channels
    if (not m_enables[chn])
      continue;

    if (not buffer.empty())
      return false;
  }

  return true;
  // Is there a way to check this from FelixClient?
}

void FelixTxCore::writeFifo(uint32_t value) {

  if (m_broadcast) {
    auto fid_broadcast = fid_from_channel(BroadcastChn);
    ftlog->trace("FelixTxCore::writeFifo link=0x{:x} val=0x{:08x}", fid_broadcast, value);
    fillFifo(m_fifo[fid_broadcast], value);

  } else {
    // write value to all enabled channels
    for (auto& [chn, buffer] : m_fifo) {
      if (m_enables[chn]) {
        ftlog->trace("FelixTxCore::writeFifo link=0x{:x} val=0x{:08x}", chn, value);
        fillFifo(buffer, value);
      }
    }
  }
}

void FelixTxCore::fillFifo(std::vector<uint8_t>& fifo, uint32_t value) {
  // Break an unsigned int into four bytes
  // MSB first
  fifo.push_back( (value>>24) & 0xff );
  fifo.push_back( (value>>16) & 0xff );
  fifo.push_back( (value>>8) & 0xff );
  fifo.push_back( value & 0xff );
}

void FelixTxCore::prepareFifo(std::vector<uint8_t>& fifo) {

  if (m_flip) {
    ftlog->trace("Swap the top and bottom four bits for every byte");
    for (uint8_t &word : fifo) {
      word = ((word & 0x0f) << 4) + ((word & 0xf0) >> 4);
    }
  }

  // padding, manchester still needed?
}

void FelixTxCore::sendFifo(FelixID_t fid, std::vector<uint8_t>& fifo) {
  ftlog->trace(" send to fid 0x{:x}", fid);

  prepareFifo(fifo);

  ftlog->trace("FIFO[{}][{}]: ", fid, fifo.size());
  for (const auto& word : fifo) {
    ftlog->trace(" {:02x}", word&0xff);
  }

  bool flush = false;
  //fclient->init_send_data(fid);
  fclient->send_data(fid, fifo.data(), fifo.size(), flush);

  // clear the fifo
  fifo.clear();
}

void FelixTxCore::releaseFifo() {
  ftlog->trace("FelixTxCore::releaseFifo");

  if (m_broadcast) {
    auto fid_broadcast = fid_from_channel(BroadcastChn);
    sendFifo(fid_broadcast, m_fifo[fid_broadcast]);

  } else {
    for (auto& [chn, buffer] : m_fifo) {
      // skip disabled channels
      if (not m_enables[chn])
        continue;

      sendFifo(chn, buffer);
    }
  }

}

void FelixTxCore::setTrigEnable(uint32_t value) {
  if (m_trigProc.joinable()) {
    m_trigProc.join();
  }

  if (value == 0) {
    m_trigEnabled = false;
  } else {
    m_trigEnabled = true;
    switch (m_trigCfg) {
    case INT_TIME:
    case EXT_TRIGGER:
      ftlog->debug("Starting trigger by time ({} seconds)", m_trigTime);
      m_trigProc = std::thread(&FelixTxCore::doTriggerTime, this);
      break;
    case INT_COUNT:
      ftlog->debug("Starting trigger by count ({} triggers)", m_trigCnt);
      m_trigProc = std::thread(&FelixTxCore::doTriggerCnt, this);
      break;
    default:
      ftlog->error("No config for trigger, aborting loop");
      m_trigEnabled = false;
      break;
    }
  }
}

uint32_t FelixTxCore::getTrigEnable() {
  return m_trigEnabled;
}

void FelixTxCore::maskTrigEnable(uint32_t value, uint32_t mask) { // never used
  return;
}

void FelixTxCore::toggleTrigAbort() {
  m_trigEnabled = false;
}

bool FelixTxCore::isTrigDone() {
  return (not m_trigEnabled and isCmdEmpty());
}

void FelixTxCore::setTrigConfig(enum TRIG_CONF_VALUE cfg) {
  m_trigCfg = cfg;
}

void FelixTxCore::setTrigFreq(double freq) {
  m_trigFreq = freq;
}

void FelixTxCore::setTrigCnt(uint32_t count) {
  m_trigCnt = count;
}

void FelixTxCore::setTrigTime(double time) {
  m_trigTime = time;
}

void FelixTxCore::setTrigWordLength(uint32_t length) {
  m_trigWordLength = length;
}

void FelixTxCore::setTrigWord(uint32_t *words, uint32_t size) {
  m_trigWords.clear();

  for (uint32_t i=0; i<size; i++) {
    m_trigWords.push_back(words[i]);
  }
}

void FelixTxCore::setTriggerLogicMask(uint32_t mask) {
  //Nothing to do yet
}

void FelixTxCore::setTriggerLogicMode(enum TRIG_LOGIC_MODE_VALUE mode) {
  //Nothing to do yet
}

void FelixTxCore::resetTriggerLogic() {
  //Nothing to do yet
}

uint32_t FelixTxCore::getTrigInCount() {
  return 0;
}

void FelixTxCore::prepareTrigger(std::vector<uint8_t>& trigFifo) {
  trigFifo.clear();

  // Need to send the last word in m_trigWords first
  // (Because of the way TriggerLoop sets up the trigger words)
  for (int j=m_trigWords.size()-1; j>=0; j--) {
    fillFifo(trigFifo, m_trigWords[j]);
  }

   prepareFifo(trigFifo);
}

void FelixTxCore::prepareTrigger() {
  if (m_broadcast) {
    auto fid_broadcast = fid_from_channel(BroadcastChn);
    prepareTrigger(m_trigFifo[fid_broadcast]);

  } else {
    for (const auto& [chn, enable] : m_enables) {
      if (not enable) continue;
      prepareTrigger(m_trigFifo[chn]);
    }
  }
}

void FelixTxCore::doTriggerCnt() {
  prepareTrigger();

  uint32_t trigs = 0;
  for (uint32_t i=0; i<m_trigCnt; i++) {
    if (not m_trigEnabled) break;
    trigs++;
    trigger();
    std::this_thread::sleep_for(std::chrono::microseconds((int)(1e6/m_trigFreq))); // Frequency in Hz
  }
  m_trigEnabled = false;
  ftlog->debug("Finished trigger count {}/{}", trigs, m_trigCnt);
}

void FelixTxCore::doTriggerTime() {
  prepareTrigger();

  std::chrono::steady_clock::time_point start = std::chrono::steady_clock::now();
  std::chrono::steady_clock::time_point cur = start;
  uint32_t trigs=0;
  while (std::chrono::duration_cast<std::chrono::seconds>(cur-start).count() < m_trigTime) {
    if (not m_trigEnabled) break;
    trigs++;
    trigger();
    std::this_thread::sleep_for(std::chrono::microseconds((int)(1000/m_trigFreq))); // Frequency in kHz
    cur = std::chrono::steady_clock::now();
  }
  m_trigEnabled = false;
  ftlog->debug("Finished trigger time {} with {} triggers", m_trigTime, trigs);
}

void FelixTxCore::trigger() {
  ftlog->trace("FelixTxCore::trigger");

  if (m_broadcast) {
    auto fid_broadcast = fid_from_channel(BroadcastChn);
    bool flush = false;
    fclient->send_data(fid_broadcast, m_trigFifo[fid_broadcast].data(), m_trigFifo[fid_broadcast].size(), flush);

  } else {
    for (auto& [chn, buffer] : m_trigFifo) {
      if (not m_enables[chn]) continue;

      bool flush = false;
      fclient->send_data(chn, buffer.data(), buffer.size(), flush);
    }
  }
}

void FelixTxCore::loadConfig(const json &j) {
  ftlog->info("FelixTxCore:");

  if (j.contains("flip")) {
    m_flip = j["flip"];
    ftlog->info(" flip = {}", m_flip);
  }

  if (j.contains("detectorID")) {
    m_did = j["detectorID"];
    ftlog->info(" did = {}", m_did);
  }
  if (j.contains("connectorID")) {
    m_cid = j["connectorID"];
    ftlog->info(" cid = {}", m_cid);
  }
  if (j.contains("protocol")) {
    m_protocol = j["protocol"];
    ftlog->info(" protocol = {}", m_protocol);
  }

  if (j.contains("broadcast")) {
    m_broadcast = j["broadcast"];
    ftlog->info(" broadcast = {}", m_broadcast);
  }
}

void FelixTxCore::writeConfig(json& j) {
  j["detectorID"] = m_did;
  j["connectorID"] = m_cid;
  j["protocol"] = m_protocol;
  j["flip"] = m_flip;
}

void FelixTxCore::setClient(std::shared_ptr<FelixClientThread> client) {
  fclient = client;
}

FelixClientThread::Reply FelixTxCore::accessFelixRegister(
  FelixClientThread::Cmd cmd, const std::vector<std::string>& cmd_args)
{
  // A dummy fid made from the correct did and cid, but arbitrary link number
  // send_cmd will map this to the proper fid for register access
  std::vector<uint64_t> fids = {FelixTxCore::fid_from_channel(42)};

  // felix-register can potentially serve multiple devices
  std::vector<FelixClientThread::Reply> replies;

  auto status_summary = fclient->send_cmd(fids, cmd, cmd_args, replies);

  if (replies.empty()) {
    ftlog->warn("Status: {}", FelixClientThread::to_string(status_summary));
    throw std::runtime_error("No replies.");
  }

  // The current setup assumes the controller only handles one FELIX device (with m_did and m_cid)
  // replies.size() should also be the same as fids.size() for send_cmd()
  assert(replies.size()==1);
  const auto& reply = replies[0];

  return reply;
}

bool FelixTxCore::checkReply(const FelixClientThread::Reply& reply) {

  bool goodReply = reply.status == FelixClientThread::Status::OK;

  if (not goodReply) {
    ftlog->warn("Status: {}", FelixClientThread::to_string(reply.status));
    ftlog->warn(reply.message);
  } else {
    //status OK
    ftlog->debug("OK from 0x{:x}", reply.ctrl_fid);
    ftlog->debug("Register value = 0x{:x}", reply.value);
    if (not reply.message.empty()) ftlog->debug("message: {}", reply.message);
  }

  return goodReply;
}

bool FelixTxCore::readFelixRegister(
  const std::string& registerName, uint64_t& value)
{
  ftlog->debug("Read FELIX register {}", registerName);

  bool success = false;

  try {
    auto reply = accessFelixRegister(FelixClientThread::Cmd::GET, {registerName});
    success = checkReply(reply);
    value = reply.value;
  } catch (std::runtime_error &e) {
    ftlog->error(e.what());
  }

  if (not success) {
    ftlog->error("Fail to read register {}", registerName);
  }

  return success;
}

bool FelixTxCore::writeFelixRegister(
  const std::string& registerName, const std::string& regValue
)
{
  ftlog->debug("Write value {} to FELIX register {}", regValue, registerName);

  bool success = false;

  try {
    auto reply = accessFelixRegister(FelixClientThread::Cmd::SET, {registerName, regValue});
    success = checkReply(reply);
  } catch (std::runtime_error &e) {
    ftlog->error(e.what());
  }

  if (not success) {
    ftlog->error("Fail to write register {}", registerName);
  }

  return success;
}