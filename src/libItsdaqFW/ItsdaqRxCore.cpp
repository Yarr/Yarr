#include "ItsdaqRxCore.h"

#include <iomanip>

const size_t max_stream = 256;

std::chrono::steady_clock::time_point bridge_watcher;

ItsdaqRxCore::ItsdaqRxCore(ItsdaqHandler&h)
  : m_h(h)
{
}

ItsdaqRxCore::~ItsdaqRxCore(){
}

void ItsdaqRxCore::init() {
  bridge_watcher = std::chrono::steady_clock::now();
}

void ItsdaqRxCore::setRxEnable(uint32_t stream) {
  // Set stream mask (fixed params, capture?)
  std::cout << "Set Rx enable only stream " << stream << '\n';

  disableRx();

  const uint16_t conf_mask = 0xffff;
  const uint16_t conf_value = 0x0001;

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
  std::cout << "Skip setRxEnable list of " << channels.size() << " streams\n";

  // First disable all
  disableRx();

  for(uint32_t c: channels) {
    setRxEnable(c);
  }

  bridge_watcher = std::chrono::steady_clock::now();
}

void ItsdaqRxCore::maskRxEnable(uint32_t val, uint32_t mask) {
  std::cout << "Skip maskRxEnable " << val << ' ' << mask << '\n';
}

void ItsdaqRxCore::flushBuffer(){
  std::cout << "Skip flushBuffer\n";
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
}

void ItsdaqRxCore::fromFileJson(json &j) {
}
