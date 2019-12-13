#include "ItsdaqRxCore.h"

#include <iomanip>

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

void ItsdaqRxCore::setRxEnable(uint32_t val) {
  // Set stream mask (fixed params, capture?)
  std::cout << "Skip setRxEnable " << val << '\n';

  bridge_watcher = std::chrono::steady_clock::now();
}

void ItsdaqRxCore::disableRx() {
  std::cout << "Skip disableRx\n";
}

void ItsdaqRxCore::setRxEnable(std::vector<uint32_t> channels) {
  std::cout << "Skip setRxEnable chan list\n";
    // this->disableAllChannels();
    // for (uint32_t channel : channels) {
    //     this->enableChannel(channel);
    // }

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
