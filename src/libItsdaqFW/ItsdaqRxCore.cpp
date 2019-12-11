#include "ItsdaqRxCore.h"

#include <iomanip>

ItsdaqRxCore::ItsdaqRxCore(ItsdaqHandler&h)
  : m_h(h)
{
}

ItsdaqRxCore::~ItsdaqRxCore(){
}

void ItsdaqRxCore::setRxEnable(uint32_t val) {
  // Set stream mask (fixed params, capture?)
  std::cout << "Skip setRxEnable " << val << '\n';
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
  // std::this_thread::sleep_for(std::chrono::milliseconds(1)); 
  return 0; // m_h.GetQueuedDataCount();
}

bool ItsdaqRxCore::isBridgeEmpty(){ // True, if queues are stable.
  return true; // m_nioh.isAllStable();
}

void ItsdaqRxCore::toFileJson(json &j) {
}

void ItsdaqRxCore::fromFileJson(json &j) {
}
