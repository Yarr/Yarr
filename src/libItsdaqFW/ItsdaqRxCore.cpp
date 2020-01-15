#include "ItsdaqRxCore.h"

#include <iomanip>

#include "logging.h"

const size_t max_stream = 256;

std::chrono::steady_clock::time_point bridge_watcher;

namespace {
auto logger = logging::make_log("ItsdaqFW::RxCore");
}

ItsdaqRxCore::ItsdaqRxCore(ItsdaqHandler&h)
  : m_h(h), m_streamConfig(0)
{
}

ItsdaqRxCore::~ItsdaqRxCore(){
}

void ItsdaqRxCore::init() {
  bridge_watcher = std::chrono::steady_clock::now();

  // Make TS pass more quickly to encourage packets to appear
  std::array<uint16_t, 2> wrReg{32, 0xc};
  m_h.SendOpcode(0x10, wrReg.data(), 2);

  const uint16_t SEND_TO_ME = 0x00f2;
  std::array<uint16_t, 2> noContents{0, 0};
  m_h.SendOpcode(SEND_TO_ME, noContents.data(), 2);
}

void ItsdaqRxCore::setRxEnable(uint32_t stream) {
  // Set stream mask (fixed params, capture?)
  logger->debug("Set Rx enable only stream {}", stream);

  disableRx();

  const uint16_t conf_mask = 0xffff;
  const uint16_t conf_value = m_streamConfig | 1;

  // Mask, stream, value
  std::array<uint16_t, 3> buffer = {conf_mask, (uint16_t)stream, conf_value};
  uint16_t opcode = 0x0050;
  m_h.SendOpcode(opcode, buffer.data(), 3);

  bridge_watcher = std::chrono::steady_clock::now();
}

void ItsdaqRxCore::disableRx() {
  // Disable all streams
  // Mask, (stream, value)*
  static const size_t length = 1 + max_stream *2;
  std::array<uint16_t, length> buffer;
  buffer[0] = 1; // Changing mask
  for(size_t s=0; s<max_stream; s++) {
    buffer[s*2+1] = s;
    buffer[s*2+2] = 0; // Mask set to off
  }

  uint16_t opcode = 0x0050;
  m_h.SendOpcode(opcode, buffer.data(), length);
}

void ItsdaqRxCore::setRxEnable(std::vector<uint32_t> channels) {
  logger->debug("Skip setRxEnable list of {} streams", channels.size());

  // First disable all
  disableRx();

  for(uint32_t c: channels) {
    setRxEnable(c);
  }

  bridge_watcher = std::chrono::steady_clock::now();
}

void ItsdaqRxCore::maskRxEnable(uint32_t val, uint32_t mask) {
  logger->debug("Skip maskRxEnable {} {}", val, mask);
}

void ItsdaqRxCore::flushBuffer(){
  logger->debug("Skip flushBuffer");
}

RawData* ItsdaqRxCore::readData(){
  return NULL;
}

uint32_t ItsdaqRxCore::getDataRate(){
  // Default HCCStar rate
  return 320;
}

uint32_t ItsdaqRxCore::getCurCount(){
  return !isBridgeEmpty();
}

bool ItsdaqRxCore::isBridgeEmpty() {
  // Something might still arrive in the future
  std::chrono::steady_clock::time_point t1 = std::chrono::steady_clock::now();
  // Assume bridge is empty if time passed
  // Intentionally shorter than 1ms otherwise could get stuck receiving HPR...
  return (t1 - bridge_watcher) > std::chrono::microseconds(100);
}

void ItsdaqRxCore::toFileJson(json &j) {
  if(m_streamConfig != 0) {
    j["streamConfig"] = m_streamConfig;
  }
}

void ItsdaqRxCore::fromFileJson(json &j) {
  if(j["streamConfig"]) {
    m_streamConfig =  j["streamConfig"];
  }
}
